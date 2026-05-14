from __future__ import annotations
from dataclasses import dataclass
from typing import Callable, Optional, Any
import logging, time
from stm32_config import STM32Config
from stm32_packets import Packet, PacketType, PacketFlags, decode_packet, encode_packet, make_heartbeat_packet, encode_key_values, decode_key_values
from stm32_state import STM32RuntimeState, TelemetrySample, ConnectionState, LinkHealth

logger = logging.getLogger(__name__)

@dataclass(slots=True)
class ProtocolEvent:
    name: str
    payload: dict[str, Any]

class STM32Protocol:
    def __init__(self, config: STM32Config, state: STM32RuntimeState, on_event: Optional[Callable[[ProtocolEvent], None]] = None) -> None:
        self.config = config; self.state = state; self.on_event = on_event; self.sequence_tx = 0

    def next_sequence(self) -> int:
        self.sequence_tx = (self.sequence_tx + 1) & 0xFFFFFFFF; return self.sequence_tx

    def _emit(self, name: str, payload: dict[str, Any]) -> None:
        if self.on_event:
            try: self.on_event(ProtocolEvent(name=name, payload=payload))
            except Exception: logger.exception("protocol event callback failed")

    def build_heartbeat(self, request: bool = False) -> bytes:
        seq = self.next_sequence(); 
        if request: self.state.heartbeat_tx()
        return make_heartbeat_packet(sequence=seq, version=self.config.protocol_version, request=request)

    def build_command(self, command_name: str, args: Optional[dict[str, Any]] = None, emergency: bool = False) -> bytes:
        seq = self.next_sequence(); self.state.record_tx(seq)
        payload = encode_key_values({"command": command_name, **(args or {})})
        flags = PacketFlags.ACK_REQUIRED | (PacketFlags.EMERGENCY if emergency else PacketFlags.NONE)
        return encode_packet(msg_type=PacketType.COMMAND, sequence=seq, payload=payload, version=self.config.protocol_version, flags=flags)

    def build_version_negotiation(self, local_name: str = "haley-linux-host") -> bytes:
        seq = self.next_sequence(); self.state.record_tx(seq)
        payload = encode_key_values({"name": local_name, "protocol_version": self.config.protocol_version, "audio_sample_rate_hz": self.config.audio_sample_rate_hz, "audio_channels": self.config.audio_channels})
        return encode_packet(msg_type=PacketType.VERSION_NEGOTIATION, sequence=seq, payload=payload, version=self.config.protocol_version, flags=PacketFlags.REQUEST)

    def build_telemetry_request(self) -> bytes:
        seq = self.next_sequence(); self.state.record_tx(seq)
        return encode_packet(msg_type=PacketType.TELEMETRY, sequence=seq, payload=b"", version=self.config.protocol_version, flags=PacketFlags.REQUEST)

    def parse(self, data: bytes) -> Packet:
        pkt = decode_packet(data, verify_crc=self.config.use_crc32); self.state.record_rx(pkt.header.sequence)
        if pkt.header.msg_type == PacketType.HEARTBEAT:
            self.state.heartbeat_rx(); self.state.connection_state = ConnectionState.CONNECTED; self.state.link_health = LinkHealth.GOOD; self._emit("heartbeat", {"sequence": pkt.header.sequence})
        elif pkt.header.msg_type == PacketType.TELEMETRY:
            telem = self.parse_telemetry(pkt.payload); self.state.update_telemetry(telem); self._emit("telemetry", telem.__dict__)
        elif pkt.header.msg_type == PacketType.FAULT:
            fields = decode_key_values(pkt.payload); self.state.mark_fault(fields.get("fault", "unknown fault")); self._emit("fault", fields)
        elif pkt.header.msg_type == PacketType.VERSION_NEGOTIATION:
            fields = decode_key_values(pkt.payload); remote = int(fields.get("protocol_version", "0") or 0)
            self.state.mark_connected(remote if remote == self.config.protocol_version else None)
            if remote != self.config.protocol_version: self.state.mark_degraded(f"Protocol mismatch: remote={remote} local={self.config.protocol_version}")
            self._emit("version", fields)
        elif pkt.header.msg_type == PacketType.ACK:
            self._emit("ack", decode_key_values(pkt.payload))
        return pkt

    def parse_telemetry(self, payload: bytes) -> TelemetrySample:
        f = decode_key_values(payload)
        return TelemetrySample(
            uptime_ms=int(f.get("uptime_ms", "0") or 0),
            temperature_c=float(f["temperature_c"]) if "temperature_c" in f else None,
            supply_voltage_v=float(f["supply_voltage_v"]) if "supply_voltage_v" in f else None,
            fault_code=int(f.get("fault_code", "0") or 0),
            mic_active=f.get("mic_active", "false").lower() in {"1","true","yes","on"},
            network_ok=f.get("network_ok", "false").lower() in {"1","true","yes","on"},
            audio_overrun=f.get("audio_overrun", "false").lower() in {"1","true","yes","on"},
            audio_underrun=f.get("audio_underrun", "false").lower() in {"1","true","yes","on"},
            extra={k:v for k,v in f.items() if k not in {"uptime_ms","temperature_c","supply_voltage_v","fault_code","mic_active","network_ok","audio_overrun","audio_underrun"}}
        )

    def is_heartbeat_due(self) -> bool:
        last = self.state.last_heartbeat_tx_s
        return last is None or (time.monotonic() - last) >= self.config.heartbeat_interval_s
