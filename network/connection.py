import json
import socket
from typing import Any, Optional


class Connection:
    """A small wrapper around a socket to send/receive JSON payloads."""

    def __init__(self, sock: Optional[socket.socket] = None):
        self.sock = sock

    def send(self, payload: Any) -> None:
        if self.sock is None:
            raise RuntimeError("No socket attached")
        data = json.dumps(payload).encode("utf-8")
        self.sock.sendall(data)

    def recv(self) -> Any:
        if self.sock is None:
            raise RuntimeError("No socket attached")
        data = self.sock.recv(4096)
        if not data:
            return None
        return json.loads(data.decode("utf-8"))

    def close(self) -> None:
        if self.sock is not None:
            self.sock.close()
            self.sock = None
