"""Event bus for the Linux AI host scaffold.

This module provides a lightweight publish/subscribe layer that keeps services
decoupled while making system-wide events easy to observe.
"""

from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass, field
from queue import Queue, Empty
from threading import RLock, Thread
from time import time
from typing import Any, Callable, DefaultDict, Optional
import logging
import uuid

logger = logging.getLogger(__name__)


@dataclass(slots=True)
class Event:
    """A single message flowing through the internal event bus."""
    topic: str
    source: str
    payload: dict[str, Any] = field(default_factory=dict)
    priority: int = 0
    correlation_id: str = field(default_factory=lambda: uuid.uuid4().hex)
    timestamp: float = field(default_factory=time)


Subscriber = Callable[[Event], None]


class EventBus:
    """Simple thread-safe pub/sub event bus."""

    def __init__(self) -> None:
        self._subscribers: DefaultDict[str, list[Subscriber]] = defaultdict(list)
        self._lock = RLock()
        self._queue: "Queue[Event]" = Queue()
        self._running = False
        self._worker: Optional[Thread] = None

    def subscribe(self, topic: str, callback: Subscriber) -> None:
        with self._lock:
            self._subscribers[topic].append(callback)
        logger.debug("Subscribed callback to topic=%s", topic)

    def unsubscribe(self, topic: str, callback: Subscriber) -> None:
        with self._lock:
            if topic in self._subscribers and callback in self._subscribers[topic]:
                self._subscribers[topic].remove(callback)
        logger.debug("Unsubscribed callback from topic=%s", topic)

    def publish(
        self,
        topic: str,
        source: str,
        payload: Optional[dict[str, Any]] = None,
        priority: int = 0,
    ) -> Event:
        event = Event(topic=topic, source=source, payload=payload or {}, priority=priority)
        if self._running:
            self._queue.put(event)
        else:
            self.dispatch(event)
        return event

    def dispatch(self, event: Event) -> None:
        with self._lock:
            callbacks = list(self._subscribers.get(event.topic, [])) + list(self._subscribers.get("*", []))

        for callback in callbacks:
            try:
                callback(event)
            except Exception:
                logger.exception("Event handler failed topic=%s source=%s", event.topic, event.source)

    def topics(self) -> list[str]:
        with self._lock:
            return sorted(self._subscribers.keys())

    def start_background_worker(self) -> None:
        if self._running:
            return
        self._running = True
        self._worker = Thread(target=self._worker_loop, name="event-bus", daemon=True)
        self._worker.start()
        logger.info("Event bus worker started")

    def stop_background_worker(self) -> None:
        self._running = False
        if self._worker and self._worker.is_alive():
            self._worker.join(timeout=1.0)
        self._worker = None
        logger.info("Event bus worker stopped")

    def _worker_loop(self) -> None:
        while self._running:
            try:
                event = self._queue.get(timeout=0.1)
            except Empty:
                continue
            self.dispatch(event)

    def clear(self) -> None:
        with self._lock:
            self._subscribers.clear()


def make_default_bus() -> EventBus:
    return EventBus()
