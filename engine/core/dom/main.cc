// bms_browser_main.cpp - BMS Browser Main Application
#include "bms_dom_engine.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>

using namespace BMS::DOM;

// ============================================================================
// BMS Browser Application
// ============================================================================

class BMSBrowserApp {
public:
    BMSBrowserApp() {
        std::cout << "╔═══════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║           BMS Browser v1.0                                ║" << std::endl;
        std::cout << "║           the ultimate browser                            ║" << std::endl;
        std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    }
    
    ~BMSBrowserApp() {
        shutdown();
    }
    
    bool initialize() {
        // Initialize browser engine
        return engine_.initialize(0, nullptr);
    }
    
    void shutdown() {
        engine_.shutdown();
    }
    
    void run() {
        // Load ignore patterns
        engine_.loadIgnorePatterns(".bmsignore");
        
        // Register APIs
        engine_.registerAPI("search", [this](const std::string& query) {
            this->onSearch(query);
        });
        
        engine_.registerAPI("navigate", [this](const std::string& url) {
            this->onNavigate(url);
        });
        
        // Create window
        engine_.createWindow("https://bms-browser.com", 1280, 720);
        
        // Search example
        engine_.search("browser engine");
        engine_.searchGlobal("chromium dom");
        
        // Run main loop
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    void onSearch(const std::string& query) {
        std::cout << "🔍 Searching: " << query << std::endl;
        engine_.search(query);
    }
    
    void onNavigate(const std::string& url) {
        std::cout << "🌐 Navigating to: " << url << std::endl;
        // Navigate in all windows
        for (auto& [handle, context] : windows_) {
            engine_.navigate(handle, url);
        }
    }
    
private:
    BMSBrowserEngine engine_;
    std::unordered_map<void*, void*> windows_;
};

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char** argv) {
    BMSBrowserApp app;
    
    if (!app.initialize()) {
        std::cerr << "Failed to initialize BMS Browser" << std::endl;
        return 1;
    }
    
    app.run();
    
    return 0;
}