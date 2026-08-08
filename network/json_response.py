import json

from network.http import HTTPResponse


class JsonResponse(HTTPResponse):
    def __init__(self, payload, status=200, headers=None):
        body = json.dumps(payload)
        merged_headers = {"Content-Type": "application/json"}
        if headers:
            merged_headers.update(headers)
        super().__init__(body=body, status_code=status, headers=merged_headers)
