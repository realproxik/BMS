# Node integration (BMS)

This folder contains a small scaffold for embedding Node into the BMS browser.

What is included:
- `NodeEmbed.h` / `NodeEmbed.cpp`: Minimal stubs showing lifecycle methods.

Next steps to implement real integration:
1. Decide whether to embed `libnode` (full Node runtime) or only V8.
2. Add third-party dependency provisioning (e.g., build libnode alongside Chromium or use prebuilt libnode).
3. Implement proper initialization, message loop integration with the browser's event loop, and security boundary for renderer/worker contexts.

Security notes:
- Exposing Node APIs to web content must be sandboxed. Prefer a dedicated IPC channel and capability-based APIs.
