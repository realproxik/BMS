import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from network.dns import DNSResolver
from network.http import HTTPRequest
from network.json_response import JsonResponse
from network.signal import Signal


def main() -> None:
    signal = Signal()
    signal.on("ready", lambda payload: print("signal:", payload))
    signal.emit("ready", {"status": "ok"})

    resolver = DNSResolver()
    print("dns:", resolver.resolve("localhost"))

    request = HTTPRequest.parse("GET /health HTTP/1.1\r\nHost: example.test\r\n\r\n")
    print("http:", request.method, request.path)

    response = JsonResponse({"message": "hello"}, status=200)
    print("json response:", response.body)


if __name__ == "__main__":
    main()
