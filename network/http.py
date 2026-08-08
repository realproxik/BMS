from dataclasses import dataclass
from typing import Dict, Optional


@dataclass
class HTTPRequest:
    method: str
    path: str
    version: str
    headers: Dict[str, str]

    @classmethod
    def parse(cls, raw: str) -> "HTTPRequest":
        lines = [line for line in raw.split("\r\n") if line]
        if not lines:
            raise ValueError("Empty request")
        method, path, version = lines[0].split(" ")
        headers = {}
        for line in lines[1:]:
            if ":" in line:
                key, value = line.split(":", 1)
                headers[key.strip()] = value.strip()
        return cls(method=method, path=path, version=version, headers=headers)


class HTTPResponse:
    def __init__(self, body: str, status_code: int = 200, headers: Optional[Dict[str, str]] = None):
        self.body = body
        self.status_code = status_code
        self.headers = headers or {}

    def to_bytes(self) -> bytes:
        return self.body.encode("utf-8")
