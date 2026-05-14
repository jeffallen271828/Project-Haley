from __future__ import annotations
from dataclasses import dataclass, field
from enum import Enum
from time import monotonic
from typing import Any

class ConnectionState(str, Enum):
    DISCONNECTED = "disconnected"
    CONNECTING = "connecting"
    HANDSHAKING = "handshaking"
    CONNECTED = "connected"
    DEGRADED = "degraded"
    FAULT = "fault"

class LinkHealth(str, Enum):
    UNKNOWN = "unknown"
    GOOD = "good"
    WARN = "warn"
    BAD = "bad"

@dataclass(slots=True)
class PacketStats:
    sent: int = 0
    received: int = 0
    crc_errors: int = 0
    parse_errors: int = 0
    dropped: int = 0
    last_sequence_rx: int = 0
    last_sequence_tx: int = 0

@dataclass(slots=True)
class TelemetrySample:
    uptime_ms: int = 0
    temperature_c: float | None = None
    supply_voltage_v: float | None = None
    fault_code: int = 0
    mic_active: bool = False
    network_ok: bool = False
    audio_overrun: bool = False
    audio_underrun: bool = False
    extra: dict[str, Any] = field(default_factory=dict)

@dataclass(slots=True)
class STM32RuntimeState:
    connection_state: ConnectionState = ConnectionState.DISCONNECTED
    link_health: LinkHealth = LinkHealth.UNKNOWN
    protocol_version: int = 1
    last_heartbeat_rx_s: float | None = None
    last_heartbeat_tx_s: float | None = None
    last_telemetry_rx_s: float | None = None
    last_command_tx_s: float | None = None
    packet_stats: PacketStats = field(default_factory=PacketStats)
    telemetry: TelemetrySample = field(default_factory=TelemetrySample)
    last_error: str | None = None
    updated_at_s: float = field(default_factory=monotonic)

    def touch(self): self.updated_at_s = monotonic()
    def mark_connected(self, protocol_version: int | None = None):
        self.connection_state = ConnectionState.CONNECTED; self.link_health = LinkHealth.GOOD
        if protocol_version is not None: self.protocol_version = protocol_version
        self.touch()
    def mark_disconnected(self, reason: str | None = None):
        self.connection_state = ConnectionState.DISCONNECTED; self.link_health = LinkHealth.UNKNOWN; self.last_error = reason; self.touch()
    def mark_degraded(self, reason: str | None = None):
        self.connection_state = ConnectionState.DEGRADED; self.link_health = LinkHealth.WARN; self.last_error = reason; self.touch()
    def mark_fault(self, reason: str | None = None):
        self.connection_state = ConnectionState.FAULT; self.link_health = LinkHealth.BAD; self.last_error = reason; self.touch()
    def record_rx(self, sequence: int):
        self.packet_stats.received += 1; self.packet_stats.last_sequence_rx = sequence; self.touch()
    def record_tx(self, sequence: int):
        self.packet_stats.sent += 1; self.packet_stats.last_sequence_tx = sequence; self.last_command_tx_s = monotonic(); self.touch()
    def record_crc_error(self): self.packet_stats.crc_errors += 1; self.touch()
    def record_parse_error(self): self.packet_stats.parse_errors += 1; self.touch()
    def record_drop(self): self.packet_stats.dropped += 1; self.touch()
    def update_telemetry(self, telemetry: TelemetrySample):
        self.telemetry = telemetry; self.last_telemetry_rx_s = monotonic(); self.touch()
    def heartbeat_rx(self): self.last_heartbeat_rx_s = monotonic(); self.touch()
    def heartbeat_tx(self): self.last_heartbeat_tx_s = monotonic(); self.touch()
    def health_snapshot(self) -> dict[str, Any]:
        return {"connection_state": self.connection_state.value, "link_health": self.link_health.value, "protocol_version": self.protocol_version, "last_error": self.last_error, "packet_stats": self.packet_stats.__dict__, "telemetry": self.telemetry.__dict__, "updated_at_s": self.updated_at_s}
