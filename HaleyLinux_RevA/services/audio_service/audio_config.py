"""Audio service configuration for the Linux host."""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path


@dataclass(slots=True)
class AudioConfig:
    """Configuration for audio capture, buffering, and routing."""

    sample_rate_hz: int = 16000
    channels: int = 1
    sample_width_bytes: int = 2
    frame_samples: int = 512

    input_device: str | None = None
    output_device: str | None = None

    capture_chunk_samples: int = 512
    max_buffer_frames: int = 128
    router_queue_depth: int = 64

    enable_vad: bool = False
    vad_threshold: float = 0.5
    enable_resample: bool = False

    log_audio_stats: bool = True
    store_audio_cache: bool = False
    audio_cache_dir: Path = field(default_factory=lambda: Path("data/audio_cache"))

    def frame_bytes(self) -> int:
        return self.frame_samples * self.channels * self.sample_width_bytes

    def chunk_bytes(self) -> int:
        return self.capture_chunk_samples * self.channels * self.sample_width_bytes

    def validate(self) -> None:
        if self.sample_rate_hz <= 0:
            raise ValueError("sample_rate_hz must be positive")
        if self.channels <= 0:
            raise ValueError("channels must be positive")
        if self.sample_width_bytes <= 0:
            raise ValueError("sample_width_bytes must be positive")
        if self.frame_samples <= 0:
            raise ValueError("frame_samples must be positive")
        if self.capture_chunk_samples <= 0:
            raise ValueError("capture_chunk_samples must be positive")
        if self.max_buffer_frames <= 0:
            raise ValueError("max_buffer_frames must be positive")
        if self.router_queue_depth <= 0:
            raise ValueError("router_queue_depth must be positive")
        if not (0.0 <= self.vad_threshold <= 1.0):
            raise ValueError("vad_threshold must be in the range [0.0, 1.0]")


DEFAULT_AUDIO_CONFIG = AudioConfig()
