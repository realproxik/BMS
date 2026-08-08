import json
import socket
import unittest

from network.connection import Connection
from network.dns import DNSResolver
from network.http import HTTPRequest, HTTPResponse
from network.json_response import JsonResponse
from network.signal import Signal


class NetworkSystemTests(unittest.TestCase):
    def test_connection_roundtrip(self):
        left, right = socket.socketpair()
        try:
            left_conn = Connection(left)
            right_conn = Connection(right)
            left_conn.send({"event": "ping"})
            payload = right_conn.recv()
            self.assertEqual(payload["event"], "ping")
        finally:
            left_conn.close()
            right_conn.close()

    def test_signal_emits_to_subscribers(self):
        signal = Signal()
        received = []
        signal.on("ready", lambda payload: received.append(payload))
        signal.emit("ready", {"status": "ok"})
        self.assertEqual(received[0]["status"], "ok")

    def test_dns_resolver_uses_localhost(self):
        resolver = DNSResolver()
        self.assertEqual(resolver.resolve("localhost"), "127.0.0.1")

    def test_http_request_and_json_response(self):
        request = HTTPRequest.parse("GET /health HTTP/1.1\r\nHost: example.test\r\n\r\n")
        self.assertEqual(request.method, "GET")
        self.assertEqual(request.path, "/health")

        response = JsonResponse({"ok": True}, status=200)
        payload = json.loads(response.body)
        self.assertTrue(payload["ok"])
        self.assertEqual(response.status_code, 200)


if __name__ == "__main__":
    unittest.main()
