"""System monitor for the Linux AI host scaffold."""

from __future__ import annotations

from dataclasses import dataclass, asdict
from time import monotonic, sleep
from typing import Any, Optional
import gc
import logging
import os
import platform

logger = logging.getLogger(__name__)

try:
    import psutil  # type: ignore
except Exception:  # optional dependency
    psutil = None


@dataclass(slots=True)
class SystemSnapshot:
    uptime_seconds: float
    platform: str
    cpu_percent: Optional[float]
    memory_percent: Optional[float]
    rss_bytes: Optional[int]
    python_gc_objects: int
    service_count: int
    healthy_services: int
    details: dict[str, Any]


class SystemMonitor:
    """Poll host and service health, then optionally publish telemetry."""

    def __init__(self, registry: Any = None, event_bus: Any = None) -> None:
        self._registry = registry
        self._event_bus = event_bus
        self._start_time = monotonic()
        self._running = False

    def snapshot(self) -> SystemSnapshot:
        uptime = monotonic() - self._start_time
        cpu_percent = None
        memory_percent = None
        rss_bytes = None

        if psutil is not None:
            try:
                proc = psutil.Process(os.getpid())
                cpu_percent = psutil.cpu_percent(interval=None)
                memory_percent = psutil.virtual_memory().percent
                rss_bytes = int(proc.memory_info().rss)
            except Exception:
                logger.exception("psutil snapshot failed")

        service_count = 0
        healthy_services = 0
        service_details: dict[str, Any] = {}

        if self._registry is not None:
            try:
                service_count = len(self._registry.names())
                health_map = self._registry.health_snapshot()
                healthy_services = sum(1 for ok in health_map.values() if ok)
                service_details = self._registry.describe()
            except Exception:
                logger.exception("Failed to read registry status")

        return SystemSnapshot(
            uptime_seconds=uptime,
            platform=platform.platform(),
            cpu_percent=cpu_percent,
            memory_percent=memory_percent,
            rss_bytes=rss_bytes,
            python_gc_objects=len(gc.get_objects()),
            service_count=service_count,
            healthy_services=healthy_services,
            details=service_details,
        )

    def publish_snapshot(self) -> SystemSnapshot:
        snap = self.snapshot()
        if self._event_bus is not None:
            try:
                self._event_bus.publish(
                    topic="system.telemetry",
                    source="system_monitor",
                    payload=asdict(snap),
                )
            except Exception:
                logger.exception("Failed to publish system snapshot")
        return snap

    def run_forever(self, interval_s: float = 5.0) -> None:
        self._running = True
        while self._running:
            self.publish_snapshot()
            sleep(interval_s)

    def stop(self) -> None:
        self._running = False
