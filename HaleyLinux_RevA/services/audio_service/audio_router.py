"""Audio routing and dispatch logic for the Linux host."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Callable, Iterable, Optional
import logging

from .audio_buffer import AudioBuffer, AudioFrame
from .audio_capture import AudioCapture
from .audio_config import AudioConfig

logger = logging.getLogger(__name__)

AudioSink = Callable[[AudioFrame], None]


@dataclass(slots=True)
class AudioRouteStats:
    frames_routed: int = 0
    bytes_routed: int = 0
    sink_errors: int = 0
    last_frame_sequence: int = -1


class AudioRouter:
    """Routes captured audio frames to one or more downstream consumers."""

    def __init__(self, config: AudioConfig, buffer: AudioBuffer) -> None:
        self.config = config
        self.buffer = buffer
        self._sinks: list[AudioSink] = []
        self.stats = AudioRouteStats()

    def register_sink(self, sink: AudioSink) -> None:
        self._sinks.append(sink)
        logger.info("Registered audio sink. total=%d", len(self._sinks))

    def unregister_sink(self, sink: AudioSink) -> None:
        if sink in self._sinks:
            self._sinks.remove(sink)
            logger.info("Unregistered audio sink. total=%d", len(self._sinks))

    def clear_sinks(self) -> None:
        self._sinks.clear()

    def route_next(self) -> AudioFrame | None:
        """Pop one frame from the buffer and dispatch it to registered sinks."""
        frame = self.buffer.pop()
        if frame is None:
            return None

        self.stats.frames_routed += 1
        self.stats.bytes_routed += len(frame.data)
        self.stats.last_frame_sequence = frame.sequence

        for sink in list(self._sinks):
            try:
                sink(frame)
            except Exception:
                self.stats.sink_errors += 1
                logger.exception("Audio sink failed for frame=%s", frame.sequence)

        return frame

    def route_available(self) -> int:
        """Drain the current buffer into all sinks."""
        routed = 0
        while True:
            frame = self.route_next()
            if frame is None:
                break
            routed += 1
        return routed

    def attach_capture_callback(self, capture: AudioCapture) -> None:
        """Connect a capture source to the router through a callback."""
        def sink(_: bytes) -> None:
            self.route_available()

        capture.on_pcm = sink

    def health_snapshot(self) -> dict[str, int]:
        return {
            "buffered_frames": len(self.buffer),
            "dropped_frames": self.buffer.dropped_frames,
            "frames_routed": self.stats.frames_routed,
            "bytes_routed": self.stats.bytes_routed,
            "sink_errors": self.stats.sink_errors,
        }
