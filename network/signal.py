from typing import Callable, Dict, List


class Signal:
    """A tiny event bus with named signals and subscribers."""

    def __init__(self):
        self._handlers: Dict[str, List[Callable[[dict], None]]] = {}

    def on(self, name: str, handler: Callable[[dict], None]) -> None:
        self._handlers.setdefault(name, []).append(handler)

    def emit(self, name: str, payload: dict) -> None:
        for handler in self._handlers.get(name, []):
            handler(payload)
