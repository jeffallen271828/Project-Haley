from __future__ import annotations
from dataclasses import dataclass
from typing import Callable, Optional, Any
import logging, queue, socket, threading, time
from stm32_config import STM32Config
from stm32_packets import Packet, PacketHeader, HEADER_SIZE
from stm32_protocol import STM32Protocol, ProtocolEvent
from stm32_state import STM32RuntimeState, ConnectionState, LinkHealth

logger = logging.getLogger(__name__)

@dataclass(slots=True)
class ClientEvent:
    name: str
    payload: dict[str, Any]

class STM32Client:
    def __init__(self, config: STM32Config, state: Optional[STM32RuntimeState] = None, on_event: Optional[Callable[[ClientEvent], None]] = None) -> None:
        self.config = config; self.config.validate()
        self.state = state or STM32RuntimeState(protocol_version=config.protocol_version)
        self.on_event = on_event
        self.protocol = STM32Protocol(config=config, state=self.state, on_event=self._handle_protocol_event)
        self._sock: Optional[socket.socket] = None
        self._lock = threading.RLock()
        self._running = False
        self._rx_thread: Optional[threading.Thread] = None
        self._heartbeat_thread: Optional[threading.Thread] = None

    def _emit(self, name: str, payload: dict[str, Any]) -> None:
        if self.on_event:
            try: self.on_event(ClientEvent(name=name, payload=payload))
            except Exception: logger.exception("client event callback failed")

    def _handle_protocol_event(self, event: ProtocolEvent) -> None:
        self._emit(event.name, event.payload)

    @property
    def connected(self) -> bool:
        return self._sock is not None and self.state.connection_state in {ConnectionState.CONNECTED, ConnectionState.HANDSHAKING}

    def connect(self) -> None:
        with self._lock:
            if self._running: return
            self.state.connection_state = ConnectionState.CONNECTING; self.state.link_health = LinkHealth.UNKNOWN; self.state.touch()
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM); s.settimeout(self.config.connect_timeout_s); s.connect((self.config.host, self.config.port)); s.settimeout(self.config.read_timeout_s)
            self._sock = s; self._running = True; self.state.connection_state = ConnectionState.HANDSHAKING
            self._rx_thread = threading.Thread(target=self._rx_loop, name="stm32-rx", daemon=True); self._rx_thread.start()
            if self.config.enable_heartbeat:
                self._heartbeat_thread = threading.Thread(target=self._heartbeat_loop, name="stm32-heartbeat", daemon=True); self._heartbeat_thread.start()
            self.send_raw(self.protocol.build_version_negotiation())
            self.state.mark_connected(self.config.protocol_version)
            self._emit("connected", {"host": self.config.host, "port": self.config.port, "protocol_version": self.config.protocol_version})

    def disconnect(self, reason: str | None = None) -> None:
        with self._lock:
            self._running = False
            if self._sock:
                try: self._sock.shutdown(socket.SHUT_RDWR)
                except Exception: pass
                try: self._sock.close()
                except Exception: pass
                self._sock = None
            self.state.mark_disconnected(reason); self._emit("disconnected", {"reason": reason or "manual disconnect"})

    def reconnect(self) -> None:
        self.disconnect("reconnect"); time.sleep(self.config.reconnect_delay_s); self.connect()

    def send_raw(self, packet_bytes: bytes) -> None:
        with self._lock:
            if not self._sock: raise RuntimeError("STM32 client is not connected")
            self._sock.sendall(packet_bytes)

    def send_packet(self, packet: Packet) -> None:
        self.send_raw(packet.to_bytes())

    def send_command(self, command_name: str, args: Optional[dict[str, Any]] = None, emergency: bool = False) -> None:
        self.send_raw(self.protocol.build_command(command_name, args=args, emergency=emergency))

    def request_telemetry(self) -> None:
        self.send_raw(self.protocol.build_telemetry_request())

    def send_heartbeat(self) -> None:
        self.send_raw(self.protocol.build_heartbeat(request=False))

    def _rx_loop(self) -> None:
        buffer = bytearray()
        while self._running:
            sock = self._sock
            if sock is None: break
            try:
                chunk = sock.recv(4096)
                if not chunk:
                    self.disconnect("peer closed connection"); break
                buffer.extend(chunk)
                self._drain_buffer(buffer)
            except socket.timeout:
                continue
            except Exception as exc:
                self.state.mark_degraded(str(exc)); self._emit("rx_error", {"error": str(exc)}); break
        self._running = False

    def _drain_buffer(self, buffer: bytearray) -> None:
        from stm32_packets import PacketHeader
        while True:
            if len(buffer) < HEADER_SIZE: return
            try:
                header = PacketHeader.from_bytes(buffer[:HEADER_SIZE])
            except Exception as exc:
                self.state.record_parse_error(); self._emit("parse_error", {"error": str(exc)}); buffer.clear(); return
            total = HEADER_SIZE + header.payload_length
            if len(buffer) < total: return
            packet_bytes = bytes(buffer[:total]); del buffer[:total]
            try:
                pkt = self.protocol.parse(packet_bytes); self._emit("packet_rx", {"msg_type": pkt.header.msg_type.name, "sequence": pkt.header.sequence, "payload_length": len(pkt.payload)})
            except Exception as exc:
                self.state.record_parse_error(); self._emit("parse_error", {"error": str(exc)})

    def _heartbeat_loop(self) -> None:
        while self._running:
            try:
                if self.protocol.is_heartbeat_due():
                    self.send_heartbeat(); self.state.heartbeat_tx()
                time.sleep(max(0.1, self.config.heartbeat_interval_s / 2.0))
            except Exception as exc:
                self.state.mark_degraded(str(exc)); self._emit("heartbeat_error", {"error": str(exc)}); break

    def start(self) -> None: self.connect()
    def stop(self) -> None: self.disconnect("stop")
    def health(self) -> dict[str, Any]: return self.state.health_snapshot()
