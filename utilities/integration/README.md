# BMS Integration Framework

This folder contains a lightweight integration layer for:

- IPC communications between browser and Node subprocesses.
- Memory accounting for browser-managed allocations.
- Node runtime embedding stubs.

## Components

- `IPCManager`: simple TCP-based IPC server to receive newline-delimited messages.
- `MemoryManager`: track named allocations and query process memory.
- `NodeEmbed`: placeholder Node runtime integration.
- `BMSIntegration`: convenience wrapper tying IPC, Node, and memory together.

## Build example

g++ -std=c++17 -O2 \
  utilities/ipc/IPCManager.cpp \
  utilities/memory/MemoryManager.cpp \
  utilities/node_integration/NodeEmbed.cpp \
  utilities/integration/BMSIntegration.cpp \
  utilities/node_integration/test_node_integration.cpp \
  -o bms_integration_test

## Notes

This is an integration scaffold. Real Node embedding and Chromium process integration require:

1. Linking `libnode`, V8, and `libuv`.
2. Connecting Chromium renderer/JS contexts to Node or IPC safely.
3. Adding sandboxing/security and proper process isolation.
4. Replacing stubbed `runScript()` with actual V8 evaluation.
