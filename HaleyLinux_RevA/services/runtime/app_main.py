from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable
import logging
import signal
import sys
import time

from event_bus import EventBus, Event
from service_registry import ServiceRegistry, NullService
from lifecycle_manager import LifecycleManager
from system_monitor import SystemMonitor

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s | %(levelname)s | %(name)s | %(message)s",
)
logger = logging.getLogger(__name__)

EXPECTED_SERVICES = [
    "audio_service",
    "whisper_service",
    "llm_service",
    "tts_service",
    "memory_service",
    "personality_service",
    "stm32_interface",
    "orchestrator",
]


@dataclass
class RuntimeContext:
    registry: ServiceRegistry
    event_bus: EventBus
    monitor: SystemMonitor
    lifecycle: LifecycleManager


def build_runtime() -> RuntimeContext:
    """Construct the runtime graph and register placeholder services."""
    event_bus = EventBus()
    registry = ServiceRegistry()

    registry.register("audio_service", NullService("audio_service"))
    registry.register("whisper_service", NullService("whisper_service"), dependencies=["audio_service"])
    registry.register("llm_service", NullService("llm_service"))
    registry.register("tts_service", NullService("tts_service"), dependencies=["llm_service"])
    registry.register("memory_service", NullService("memory_service"))
    registry.register("personality_service", NullService("personality_service"), dependencies=["memory_service"])
    registry.register("stm32_interface", NullService("stm32_interface"))
    registry.register(
        "orchestrator",
        NullService("orchestrator"),
        dependencies=[
            "audio_service",
            "whisper_service",
            "llm_service",
            "tts_service",
            "memory_service",
            "personality_service",
            "stm32_interface",
        ],
    )

    monitor = SystemMonitor(registry=registry, event_bus=event_bus)
    lifecycle = LifecycleManager(registry=registry, event_bus=event_bus, monitor=monitor)
    return RuntimeContext(registry=registry, event_bus=event_bus, monitor=monitor, lifecycle=lifecycle)


def install_event_logging(bus: EventBus) -> None:
    """Attach a simple logger to all bus events."""
    def log_event(event: Event) -> None:
        logger.info("EVENT topic=%s source=%s payload=%s", event.topic, event.source, event.payload)
    bus.subscribe("*", log_event)


def main() -> int:
    """Start the Linux host scaffold."""
    logger.info("Starting Haley Linux host scaffold")

    runtime = build_runtime()
    install_event_logging(runtime.event_bus)

    def handle_signal(signum, frame) -> None:
        logger.info("Received signal=%s, shutting down", signum)
        try:
            runtime.lifecycle.shutdown()
        finally:
            sys.exit(0)

    signal.signal(signal.SIGINT, handle_signal)
    signal.signal(signal.SIGTERM, handle_signal)

    logger.info("Registered services: %s", runtime.registry.names())
    logger.info("Expected services: %s", EXPECTED_SERVICES)

    runtime.lifecycle.start()
    logger.info("Runtime started. Placeholder services are active.")

    try:
        while True:
            snapshot = runtime.monitor.publish_snapshot()
            logger.info(
                "Health: %s/%s services healthy | uptime=%.1fs",
                snapshot.healthy_services,
                snapshot.service_count,
                snapshot.uptime_seconds,
            )
            time.sleep(10.0)
    except KeyboardInterrupt:
        logger.info("Keyboard interrupt received.")
    finally:
        runtime.lifecycle.shutdown()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
