#include "NodeEmbed.h"
#include <iostream>

struct NodeEmbed::Impl {
    bool initialized = false;
};

NodeEmbed::NodeEmbed() : impl_(new Impl()) {}

NodeEmbed::~NodeEmbed() { shutdown(); }

bool NodeEmbed::init(const std::string& argv0)
{
    // Placeholder: real implementation requires libnode initialization and option parsing.
    std::cerr << "NodeEmbed::init: placeholder — link libnode and implement init()" << std::endl;
    impl_->initialized = true;
    return true;
}

bool NodeEmbed::runScript(const std::string& script)
{
    if (!impl_->initialized) return false;
    std::cerr << "NodeEmbed::runScript: executing script (placeholder)\n";
    // Real implementation would compile and run script in Node/V8.
    return true;
}

void NodeEmbed::shutdown()
{
    if (impl_ && impl_->initialized) {
        impl_->initialized = false;
        std::cerr << "NodeEmbed::shutdown: placeholder\n";
    }
}
