class DNSResolver:
    """A minimal resolver that maps localhost to loopback."""

    def resolve(self, host: str) -> str:
        host = host.strip().lower()
        if host in {"localhost", "127.0.0.1", "::1"}:
            return "127.0.0.1"
        return host
