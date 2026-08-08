// api.hh - C++ Interface Header (alternative style)
#ifndef BMS_API_HH
#define BMS_API_HH

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <cstdint>

namespace BMS {
namespace API {

// ============================================================================
// Type Definitions
// ============================================================================

using String = std::string;
using StringMap = std::unordered_map<String, String>;
using StringVector = std::vector<String>;
using Callback = std::function<void()>;
using ErrorCallback = std::function<void(int32_t, const String&)>;

// ============================================================================
// Enumerations
// ============================================================================

enum class BrowserStatus : uint8_t {
    INITIALIZING,
    RUNNING,
    SHUTTING_DOWN,
    SHUTDOWN
};

enum class NavigationType : uint8_t {
    LINK_CLICK,
    TYPED,
    BOOKMARK,
    HISTORY,
    RELOAD,
    FORWARD_BACK
};

// ============================================================================
// Interfaces
// ============================================================================

class IBrowser;
class IWindow;
class ITab;
class IDocument;
class IElement;
class INetworkRequest;
class IStorage;
class ICookie;
class IExtension;

/**
 * @brief Main browser interface (HH style)
 */
class IBrowser {
public:
    virtual ~IBrowser() = default;
    
    // Lifecycle
    virtual bool Init(int argc, char** argv) = 0;
    virtual void Shutdown() = 0;
    virtual BrowserStatus GetStatus() const = 0;
    
    // Window management
    virtual IWindow* CreateWindow(const String& url, int width = 1024, int height = 768) = 0;
    virtual void CloseWindow(IWindow* window) = 0;
    virtual std::vector<IWindow*> GetWindows() const = 0;
    virtual IWindow* GetActiveWindow() const = 0;
    
    // Tab management
    virtual ITab* CreateTab(const String& url, IWindow* window = nullptr) = 0;
    virtual void CloseTab(ITab* tab) = 0;
    virtual std::vector<ITab*> GetTabs() const = 0;
    virtual ITab* GetActiveTab() const = 0;
    
    // Navigation
    virtual void Navigate(const String& url, ITab* tab = nullptr) = 0;
    virtual void GoBack(ITab* tab = nullptr) = 0;
    virtual void GoForward(ITab* tab = nullptr) = 0;
    virtual void Refresh(ITab* tab = nullptr) = 0;
    virtual void Stop(ITab* tab = nullptr) = 0;
    
    // Events
    virtual void OnPageLoad(std::function<void(const String&)> callback) = 0;
    virtual void OnError(ErrorCallback callback) = 0;
};

// ============================================================================
// Factory Functions (HH style)
// ============================================================================

/**
 * @brief Create a browser instance
 */
std::unique_ptr<IBrowser> CreateBrowser();

/**
 * @brief Get the global browser instance
 */
IBrowser* GetBrowser();

/**
 * @brief Initialize the API
 */
bool InitAPI();

/**
 * @brief Shutdown the API
 */
void ShutdownAPI();

} // namespace API
} // namespace BMS

#endif // BMS_API_HH