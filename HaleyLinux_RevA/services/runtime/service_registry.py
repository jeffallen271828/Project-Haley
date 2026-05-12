"""Service registry for the Linux AI host scaffold."""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum
from typing import Any, Dict, Iterable, Optional, Protocol
import logging

logger = logging.getLogger(__name__)


class ServiceStatus(str, Enum):
    STOPPED = "stopped"
    STARTING = "starting"
    RUNNING = "running"
    DEGRADED = "degraded"
    STOPPING = "stopping"
    FAILED = "failed"


class ManagedService(Protocol):
    """Minimal service contract expected by the registry."""
    name: str
    def start(self) -> None: ...
    def stop(self) -> None: ...
    def health_check(self) -> bool: ...


@dataclass
class ServiceRecord:
    """Registry metadata for a single service."""
    name: str
    service: ManagedService
    dependencies: list[str] = field(default_factory=list)
    status: ServiceStatus = ServiceStatus.STOPPED
    last_error: Optional[str] = None


class NullService:
    """Placeholder service useful during scaffolding."""
    def __init__(self, name: str) -> None:
        self.name = name
        self._running = False
    def start(self) -> None:
        self._running = True
    def stop(self) -> None:
        self._running = False
    def health_check(self) -> bool:
        return self._running


class ServiceRegistry:
    """Tracks all runtime services and their relationships."""

    def __init__(self) -> None:
        self._services: Dict[str, ServiceRecord] = {}

    def register(self, name: str, service: ManagedService, dependencies: Optional[Iterable[str]] = None) -> None:
        if name in self._services:
            raise ValueError(f"Service already registered: {name}")
        record = ServiceRecord(name=name, service=service, dependencies=list(dependencies or []))
        self._services[name] = record
        logger.info("Registered service=%s deps=%s", name, record.dependencies)

    def get(self, name: str) -> ServiceRecord:
        return self._services[name]

    def names(self) -> list[str]:
        return list(self._services.keys())

    def records(self) -> list[ServiceRecord]:
        return list(self._services.values())

    def dependency_order(self) -> list[str]:
        ordered: list[str] = []
        temporary: set[str] = set()
        permanent: set[str] = set()

        def visit(node: str) -> None:
            if node in permanent:
                return
            if node in temporary:
                raise RuntimeError(f"Circular dependency detected at {node}")
            temporary.add(node)
            for dep in self._services[node].dependencies:
                if dep not in self._services:
                    raise KeyError(f"Missing dependency {dep} for {node}")
                visit(dep)
            temporary.remove(node)
            permanent.add(node)
            ordered.append(node)

        for name in self._services:
            visit(name)

        return ordered

    def start_all(self) -> None:
        for name in self.dependency_order():
            record = self._services[name]
            if record.status == ServiceStatus.RUNNING:
                continue
            logger.info("Starting service=%s", name)
            record.status = ServiceStatus.STARTING
            try:
                record.service.start()
                record.status = ServiceStatus.RUNNING
                record.last_error = None
            except Exception as exc:
                record.status = ServiceStatus.FAILED
                record.last_error = str(exc)
                logger.exception("Service failed to start: %s", name)
                raise

    def stop_all(self) -> None:
        for name in reversed(self.dependency_order()):
            record = self._services[name]
            if record.status == ServiceStatus.STOPPED:
                continue
            logger.info("Stopping service=%s", name)
            record.status = ServiceStatus.STOPPING
            try:
                record.service.stop()
                record.status = ServiceStatus.STOPPED
            except Exception as exc:
                record.status = ServiceStatus.FAILED
                record.last_error = str(exc)
                logger.exception("Service failed to stop: %s", name)

    def health_snapshot(self) -> dict[str, bool]:
        return {name: record.service.health_check() for name, record in self._services.items()}

    def describe(self) -> dict[str, dict[str, Any]]:
        return {
            name: {
                "dependencies": record.dependencies,
                "status": record.status.value,
                "last_error": record.last_error,
            }
            for name, record in self._services.items()
        }
