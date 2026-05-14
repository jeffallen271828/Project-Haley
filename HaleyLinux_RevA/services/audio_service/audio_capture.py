"""Audio capture abstraction for the Linux host.

This module is intentionally lightweight and backend-agnostic. The preferred
runtime can be sounddevice or PyAudio, but the service interface is kept simple
so the rest of the system does not depend on a specific capture library.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Callable, Optional
import logging
import threading
import time

from .audio_config import AudioConfig
from .audio_buffer import AudioBuffer

logger = logging.getLogger(__name__)


PCMCallback = Callable[[bytes], None]


@dataclass(slots=True)
class CaptureStats:
    frames_captured: int = 0
    bytes_captured: int = 0
    start_time_s: float | None = None
    last_frame_time_s: float | None = None


class AudioCapture:
    """Capture audio from a local device or injected test source."""

    def __init__(
        self,
        config: AudioConfig,
        buffer: AudioBuffer,
        on_pcm: Optional[PCMCallback] = None,
    ) -> None:
        self.config = config
        self.config.validate()
        self.buffer = buffer
        self.on_pcm = on_pcm

        self._running = False
        self._thread: threading.Thread | None = None
        self.stats = CaptureStats()

    @property
    def running(self) -> bool:
        return self._running

    def start(self) -> None:
        """Start the capture loop.

        This scaffold does not bind to a specific audio backend yet. The capture
        loop is ready for a future backend implementation or test injector.
        """
        if self._running:
            return

        self._running = True
        self.stats.start_time_s = time.monotonic()
        self._thread = threading.Thread(target=self._capture_loop, name="audio-capture", daemon=True)
        self._thread.start()
        logger.info("Audio capture started")

    def stop(self) -> None:
        self._running = False
        if self._thread and self._thread.is_alive():
            self._thread.join(timeout=1.0)
        self._thread = None
        logger.info("Audio capture stopped")

    def inject_pcm(self, pcm: bytes, *, timestamp_s: float | None = None) -> None:
        """Push a PCM block directly into the buffer.

        This is useful for early testing before a real audio backend is attached.
        """
        frame = self.buffer.push(
            pcm,
            sample_rate_hz=self.config.sample_rate_hz,
            channels=self.config.channels,
            sample_width_bytes=self.config.sample_width_bytes,
            timestamp_s=timestamp_s,
        )
        self.stats.frames_captured += 1
        self.stats.bytes_captured += len(pcm)
        self.stats.last_frame_time_s = frame.timestamp_s

        if self.on_pcm is not None:
            try:
                self.on_pcm(pcm)
            except Exception:
                logger.exception("PCM callback failed")

    def _capture_loop(self) -> None:
        """Placeholder capture loop.

        Replace this with a backend such as:
        - sounddevice.InputStream
        - PyAudio stream
        - ALSA capture wrapper
        """
        chunk = bytes(self.config.chunk_bytes())
        while self._running:
            self.inject_pcm(chunk)
            time.sleep(0.05)
