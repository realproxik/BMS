#pragma once

#include "../node_integration/NodeEmbed.h"
#include "../ipc/IPCManager.h"
#include "../memory/MemoryManager.h"
#include <string>
#include <functional>

namespace BMS {
namespace Integration {

class BMSIntegration {
public:
    BMSIntegration();
    ~BMSIntegration();

    bool initialize();
    bool startIPCServer(int minPort = 40000, int maxPort = 41000);
    bool initializeNode(const std::string& argv0 = "bms-node");

    bool runNodeScript(const std::string& script);
    bool sendIPCMessage(const std::string& message);

    void setIPCMessageCallback(IPC::IPCManager::MessageCallback callback);
    Memory::MemoryUsage getMemoryUsage() const;
    std::size_t getProcessMemoryBytes() const;

    void shutdown();

private:
    NodeEmbed node_;
    IPC::IPCManager ipc_;
    Memory::MemoryManager memory_;
    bool initialized_;
};

} // namespace Integration
} // namespace BMS
