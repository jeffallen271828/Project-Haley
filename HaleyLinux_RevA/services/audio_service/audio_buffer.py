"""Audio buffering utilities for the Linux host."""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass, field
from threading import Lock
from time import monotonic
from typing import Deque, Iterable, Optional


@dataclass(slots=True)
class AudioFrame:
    """One chunk of PCM audio."""
    timestamp_s: float
    data: bytes
    sample_rate_hz: int
    channels: int
    sample_width_bytes: int
    sequence: int = 0
    metadata: dict[str, str] = field(default_factory=dict)

    @property
    def byte_length(self) -> int:
        return len(self.data)


class AudioBuffer:
    """Thread-safe FIFO buffer for audio frames."""

    def __init__(self, max_frames: int = 128) -> None:
        if max_frames <= 0:
            raise ValueError("max_frames must be positive")
        self._frames: Deque[AudioFrame] = deque(maxlen=max_frames)
        self._lock = Lock()
        self._next_sequence = 0
        self._dropped_frames = 0

    def push(
        self,
        data: bytes,
        *,
        sample_rate_hz: int,
        channels: int,
        sample_width_bytes: int,
        timestamp_s: float | None = None,
        metadata: Optional[dict[str, str]] = None,
    ) -> AudioFrame:
        """Append a PCM chunk and return the created frame."""
        frame = AudioFrame(
            timestamp_s=timestamp_s if timestamp_s is not None else monotonic(),
            data=bytes(data),
            sample_rate_hz=sample_rate_hz,
            channels=channels,
            sample_width_bytes=sample_width_bytes,
            sequence=self._next_sequence,
            metadata=dict(metadata or {}),
        )
        self._next_sequence += 1

        with self._lock:
            if len(self._frames) == self._frames.maxlen:
                self._dropped_frames += 1
            self._frames.append(frame)

        return frame

    def push_frame(self, frame: AudioFrame) -> None:
        with self._lock:
            if len(self._frames) == self._frames.maxlen:
                self._dropped_frames += 1
            self._frames.append(frame)

    def pop(self) -> AudioFrame | None:
        with self._lock:
            if not self._frames:
                return None
            return self._frames.popleft()

    def pop_many(self, count: int) -> list[AudioFrame]:
        if count <= 0:
            return []
        out: list[AudioFrame] = []
        for _ in range(count):
            frame = self.pop()
            if frame is None:
                break
            out.append(frame)
        return out

    def peek(self) -> AudioFrame | None:
        with self._lock:
            return self._frames[0] if self._frames else None

    def clear(self) -> None:
        with self._lock:
            self._frames.clear()

    def __len__(self) -> int:
        with self._lock:
            return len(self._frames)

    @property
    def dropped_frames(self) -> int:
        return self._dropped_frames

    def to_bytes(self) -> bytes:
        """Concatenate the current buffer contents into one byte blob."""
        with self._lock:
            return b"".join(frame.data for frame in self._frames)

    def iter_frames(self) -> Iterable[AudioFrame]:
        with self._lock:
            return list(self._frames)
