#include "NodeEmbed.h"
#include "../memory/MemoryManager.h"
#include "../ipc/IPCManager.h"
#include <iostream>
#include <thread>

int main() {
    BMS::Memory::MemoryManager memoryManager;
    memoryManager.addAllocation("tab_pool", 128 * 1024 * 1024);

    BMS::NodeEmbed node;
    if (!node.init("node")) {
        std::cerr << "Failed to initialize Node integration" << std::endl;
        return 1;
    }

    if (!node.runScript("const value = 42; value * 2;")) {
        std::cerr << "Node script execution failed" << std::endl;
        return 1;
    }

    auto usage = memoryManager.getUsage();
    std::cout << "Memory usage current: " << usage.currentBytes << " bytes" << std::endl;
    std::cout << "Memory usage peak: " << usage.peakBytes << " bytes" << std::endl;

    node.shutdown();
    return 0;
}
