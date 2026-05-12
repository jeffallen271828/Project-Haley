"""Lifecycle manager for the Linux AI host scaffold."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any
import logging

logger = logging.getLogger(__name__)


@dataclass
class LifecycleResult:
    started: bool
    message: str


class LifecycleManager:
    """Coordinates system startup, shutdown, and health transitions."""

    def __init__(self, registry: Any, event_bus: Any, monitor: Any) -> None:
        self._registry = registry
        self._event_bus = event_bus
        self._monitor = monitor
        self._started = False

    @property
    def started(self) -> bool:
        return self._started

    def start(self) -> LifecycleResult:
        if self._started:
            return LifecycleResult(True, "Already started")

        logger.info("Starting Linux host lifecycle")
        self._event_bus.start_background_worker()

        try:
            self._registry.start_all()
            self._monitor.publish_snapshot()
            self._event_bus.publish(
                topic="system.state",
                source="lifecycle_manager",
                payload={"state": "started"},
            )
            self._started = True
            return LifecycleResult(True, "Started successfully")
        except Exception as exc:
            logger.exception("Lifecycle start failed")
            self._event_bus.publish(
                topic="system.state",
                source="lifecycle_manager",
                payload={"state": "failed_to_start", "error": str(exc)},
            )
            raise

    def shutdown(self) -> LifecycleResult:
        if not self._started:
            return LifecycleResult(True, "Already stopped")

        logger.info("Shutting down Linux host lifecycle")
        try:
            self._event_bus.publish(
                topic="system.state",
                source="lifecycle_manager",
                payload={"state": "stopping"},
            )
            self._registry.stop_all()
            self._monitor.stop()
            self._event_bus.stop_background_worker()
            self._started = False
            return LifecycleResult(True, "Stopped successfully")
        except Exception:
            logger.exception("Lifecycle shutdown failed")
            raise

    def restart(self) -> LifecycleResult:
        self.shutdown()
        return self.start()
