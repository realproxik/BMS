#include "MemoryManager.h"
#include <algorithm>
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#else
#include <unistd.h>
#endif

namespace BMS {
namespace Memory {

MemoryManager::MemoryManager() : peakBytes_(0) {}

MemoryManager::~MemoryManager() = default;

void MemoryManager::addAllocation(const std::string& tag, std::size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    allocations_[tag] += bytes;
    const auto current = getUsage().currentBytes;
    peakBytes_ = std::max(peakBytes_, current);
}

void MemoryManager::releaseAllocation(const std::string& tag, std::size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = allocations_.find(tag);
    if (it != allocations_.end()) {
        if (bytes >= it->second) {
            allocations_.erase(it);
        } else {
            it->second -= bytes;
        }
    }
}

void MemoryManager::clearAllocations() {
    std::lock_guard<std::mutex> lock(mutex_);
    allocations_.clear();
}

MemoryUsage MemoryManager::getUsage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    MemoryUsage usage;
    usage.currentBytes = getProcessMemoryInternal();
    for (const auto& entry : allocations_) {
        usage.currentBytes += entry.second;
    }
    usage.peakBytes = std::max(peakBytes_, usage.currentBytes);
    return usage;
}

std::size_t MemoryManager::getProcessMemoryBytes() const {
    return getProcessMemoryInternal();
}

std::size_t MemoryManager::getProcessMemoryInternal() const {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
        return static_cast<std::size_t>(pmc.WorkingSetSize);
    }
    return 0;
#else
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            std::string value = line.substr(6);
            std::size_t kb = std::stoull(value);
            return kb * 1024;
        }
    }
    return 0;
#endif
}

} // namespace Memory
} // namespace BMS
