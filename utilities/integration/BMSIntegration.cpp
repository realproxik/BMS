#include "BMSIntegration.h"
#include <iostream>

namespace BMS {
namespace Integration {

BMSIntegration::BMSIntegration() : initialized_(false) {}

BMSIntegration::~BMSIntegration() {
    shutdown();
}

bool BMSIntegration::initialize() {
    if (initialized_) return true;
    std::cout << "[BMS Integration] Initializing subsystems..." << std::endl;
    initialized_ = true;
    return true;
}

bool BMSIntegration::startIPCServer(int minPort, int maxPort) {
    if (!initialized_) return false;
    return ipc_.startServer(minPort, maxPort);
}

bool BMSIntegration::initializeNode(const std::string& argv0) {
    if (!initialized_) return false;
    return node_.init(argv0);
}

bool BMSIntegration::runNodeScript(const std::string& script) {
    if (!initialized_) return false;
    return node_.runScript(script);
}

bool BMSIntegration::sendIPCMessage(const std::string& message) {
    if (!initialized_) return false;
    return ipc_.sendMessage(message + "\n");
}

void BMSIntegration::setIPCMessageCallback(IPC::IPCManager::MessageCallback callback) {
    ipc_.setMessageCallback(std::move(callback));
}

Memory::MemoryUsage BMSIntegration::getMemoryUsage() const {
    return memory_.getUsage();
}

std::size_t BMSIntegration::getProcessMemoryBytes() const {
    return memory_.getProcessMemoryBytes();
}

void BMSIntegration::shutdown() {
    node_.shutdown();
    ipc_.stopServer();
    memory_.clearAllocations();
    initialized_ = false;
}

} // namespace Integration
} // namespace BMS
