// __init.h - C++ Initialization Header
#ifndef BMS_INIT_H
#define BMS_INIT_H

#include <string>
#include <vector>
#include <functional>

namespace BMS {
namespace Init {

// ============================================================================
// Initialization Configuration
// ============================================================================

/**
 * @brief Browser initialization configuration
 */
struct Config {
    std::string userAgent = "BMS-Browser/1.1";
    std::string homepage = "about:blank";
    std::string downloadPath = "./downloads";
    std::string cachePath = "./cache";
    int maxConnections = 10;
    int timeoutMs = 30000;
    bool enableJavaScript = true;
    bool enableCookies = true;
    bool enableCache = true;
    bool enableExtensions = true;
    bool enableGPU = true;
    bool enableWebGL = true;
    bool enableWebRTC = true;
    bool enableWebAssembly = true;
    bool enableWebWorkers = true;
    bool enableServiceWorkers = true;
    bool enableNotifications = true;
    bool enableGeolocation = true;
    bool enableMicrophone = true;
    bool enableCamera = true;
    bool enableClipboard = true;
    bool enableFullscreen = true;
    bool enableIncognito = false;
    bool enableAutoUpdate = true;
    bool enableCrashReporting = true;
    bool enableMetrics = false;
};

// ============================================================================
// Initialization Functions
// ============================================================================

/**
 * @brief Initialize the BMS browser
 * @param config Configuration (optional)
 * @return true if initialization succeeded
 */
bool initialize(const Config& config = Config());

/**
 * @brief Shutdown the BMS browser
 */
void shutdown();

/**
 * @brief Check if the browser is initialized
 * @return true if initialized
 */
bool isInitialized();

/**
 * @brief Get the current configuration
 * @return Current configuration
 */
Config getConfig();

/**
 * @brief Update configuration
 * @param config New configuration
 * @return true if update succeeded
 */
bool updateConfig(const Config& config);

// ============================================================================
// Callback Functions
// ============================================================================

/**
 * @brief Callback for initialization progress
 */
using InitProgressCallback = std::function<void(int percent, const std::string& message)>;

/**
 * @brief Callback for initialization completion
 */
using InitCompleteCallback = std::function<void(bool success, const std::string& error)>;

/**
 * @brief Register initialization callbacks
 * @param progress Progress callback
 * @param complete Complete callback
 */
void registerInitCallbacks(InitProgressCallback progress, InitCompleteCallback complete);

/**
 * @brief Remove initialization callbacks
 */
void unregisterInitCallbacks();

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Get the initialization status
 * @return Status message
 */
std::string getInitStatus();

/**
 * @brief Get initialization errors
 * @return Vector of error messages
 */
std::vector<std::string> getInitErrors();

/**
 * @brief Reset initialization state
 */
void resetInitState();

} // namespace Init
} // namespace BMS

#endif // BMS_INIT_H