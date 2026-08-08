#pragma once

#include <string>
#include <memory>

// Minimal Node embed stub. Real embedding requires linking libnode/libuv and V8.
class NodeEmbed {
public:
    NodeEmbed();
    ~NodeEmbed();

    // Initialize Node runtime. Returns true on success.
    bool init(const std::string& argv0 = "bms-node");

    // Run an inline JavaScript string.
    bool runScript(const std::string& script);

    // Shutdown runtime and free resources.
    void shutdown();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
