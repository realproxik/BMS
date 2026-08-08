#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <cstddef>

namespace BMS {
namespace Memory {

struct MemoryUsage {
    std::size_t currentBytes = 0;
    std::size_t peakBytes = 0;
};

class MemoryManager {
public:
    MemoryManager();
    ~MemoryManager();

    void addAllocation(const std::string& tag, std::size_t bytes);
    void releaseAllocation(const std::string& tag, std::size_t bytes);
    void clearAllocations();

    MemoryUsage getUsage() const;
    std::size_t getProcessMemoryBytes() const;

private:
    std::size_t getProcessMemoryInternal() const;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::size_t> allocations_;
    std::size_t peakBytes_;
};

} // namespace Memory
} // namespace BMS
