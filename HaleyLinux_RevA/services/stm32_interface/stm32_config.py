from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path

@dataclass(slots=True)
class STM32Config:
    host: str = "192.168.1.50"
    port: int = 5005
    bind_host: str = "0.0.0.0"
    bind_port: int = 0
    connect_timeout_s: float = 5.0
    read_timeout_s: float = 1.0
    heartbeat_interval_s: float = 2.0
    reconnect_delay_s: float = 2.0
    max_reconnect_delay_s: float = 15.0
    protocol_version: int = 1
    mtu_payload_bytes: int = 1400
    max_packet_bytes: int = 4096
    max_queue_depth: int = 64
    use_crc32: bool = True
    audio_sample_rate_hz: int = 16000
    audio_channels: int = 1
    audio_sample_width_bytes: int = 2
    audio_frame_samples: int = 512
    telemetry_poll_interval_s: float = 5.0
    heartbeat_timeout_s: float = 6.0
    log_packets: bool = True
    enable_reconnect: bool = True
    enable_heartbeat: bool = True
    storage_dir: Path = field(default_factory=lambda: Path("data/stm32_interface"))

    def validate(self) -> None:
        if not (0 < self.port <= 65535): raise ValueError("port must be 1..65535")
        if self.protocol_version <= 0: raise ValueError("protocol_version must be positive")
        if self.max_packet_bytes < self.mtu_payload_bytes: raise ValueError("max_packet_bytes must be >= mtu_payload_bytes")

DEFAULT_STM32_CONFIG = STM32Config()
