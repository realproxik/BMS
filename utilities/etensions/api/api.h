// api.h - Main API Header for BMS Browser
#ifndef BMS_API_H
#define BMS_API_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace BMS {
namespace API {

// Forward declarations
class Browser;
class Tab;
class Window;
class Document;
class Element;
class NetworkRequest;
class Storage;
class Cookie;
class Extension;

// ============================================================================
// Core API Interfaces
// ============================================================================

/**
 * @brief Main browser interface
 * @version 1.0.0
 */
class IBrowser {
public:
    virtual ~IBrowser() = default;
    
    // Browser lifecycle
    virtual bool initialize(int argc, char** argv) = 0;
    virtual void shutdown() = 0;
    virtual bool isRunning() const = 0;
    
    // Window management
    virtual Window* createWindow(const std::string& url, 
                                 int width = 1024, 
                                 int height = 768) = 0;
    virtual void closeWindow(Window* window) = 0;
    virtual std::vector<Window*> getWindows() const = 0;
    virtual Window* getActiveWindow() const = 0;
    
    // Tab management
    virtual Tab* createTab(const std::string& url, Window* window = nullptr) = 0;
    virtual void closeTab(Tab* tab) = 0;
    virtual std::vector<Tab*> getTabs() const = 0;
    virtual Tab* getActiveTab() const = 0;
    
    // Navigation
    virtual void navigate(const std::string& url, Tab* tab = nullptr) = 0;
    virtual void goBack(Tab* tab = nullptr) = 0;
    virtual void goForward(Tab* tab = nullptr) = 0;
    virtual void refresh(Tab* tab = nullptr) = 0;
    virtual void stop(Tab* tab = nullptr) = 0;
    
    // History
    virtual void addToHistory(const std::string& url, const std::string& title) = 0;
    virtual std::vector<std::string> getHistory() const = 0;
    virtual void clearHistory() = 0;
    
    // Bookmarks
    virtual void addBookmark(const std::string& url, const std::string& title) = 0;
    virtual void removeBookmark(const std::string& url) = 0;
    virtual std::unordered_map<std::string, std::string> getBookmarks() const = 0;
    
    // Extensions
    virtual bool installExtension(const std::string& path) = 0;
    virtual void uninstallExtension(const std::string& id) = 0;
    virtual std::vector<Extension*> getExtensions() const = 0;
    
    // Settings
    virtual void setSetting(const std::string& key, const std::string& value) = 0;
    virtual std::string getSetting(const std::string& key) const = 0;
    virtual void saveSettings() = 0;
    virtual void loadSettings() = 0;
    
    // Events
    virtual void onPageLoad(std::function<void(const std::string&)> callback) = 0;
    virtual void onTabOpen(std::function<void(Tab*)> callback) = 0;
    virtual void onTabClose(std::function<void(Tab*)> callback) = 0;
    virtual void onError(std::function<void(int, const std::string&)> callback) = 0;
};

/**
 * @brief Window interface
 */
class Window {
public:
    virtual ~Window() = default;
    
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void maximize() = 0;
    virtual void minimize() = 0;
    virtual void restore() = 0;
    virtual bool isVisible() const = 0;
    virtual bool isMaximized() const = 0;
    virtual bool isMinimized() const = 0;
    
    virtual void setTitle(const std::string& title) = 0;
    virtual std::string getTitle() const = 0;
    virtual void setSize(int width, int height) = 0;
    virtual void getSize(int& width, int& height) const = 0;
    virtual void setPosition(int x, int y) = 0;
    virtual void getPosition(int& x, int& y) const = 0;
    
    virtual void setFullscreen(bool fullscreen) = 0;
    virtual bool isFullscreen() const = 0;
    
    virtual void addTab(Tab* tab) = 0;
    virtual void removeTab(Tab* tab) = 0;
    virtual std::vector<Tab*> getTabs() const = 0;
    virtual Tab* getActiveTab() const = 0;
    virtual void setActiveTab(Tab* tab) = 0;
    
    virtual void* getNativeHandle() const = 0;
};

/**
 * @brief Tab interface
 */
class Tab {
public:
    virtual ~Tab() = default;
    
    virtual void loadURL(const std::string& url) = 0;
    virtual std::string getURL() const = 0;
    virtual std::string getTitle() const = 0;
    
    virtual void reload() = 0;
    virtual void stop() = 0;
    virtual void goBack() = 0;
    virtual void goForward() = 0;
    virtual bool canGoBack() const = 0;
    virtual bool canGoForward() const = 0;
    
    virtual Document* getDocument() const = 0;
    virtual void executeScript(const std::string& script) = 0;
    virtual std::string evaluateScript(const std::string& script) = 0;
    
    virtual void setZoomLevel(double level) = 0;
    virtual double getZoomLevel() const = 0;
    
    virtual void setMuted(bool muted) = 0;
    virtual bool isMuted() const = 0;
    virtual bool isLoading() const = 0;
    virtual double getLoadProgress() const = 0;
    
    virtual void onLoadStart(std::function<void()> callback) = 0;
    virtual void onLoadFinish(std::function<void()> callback) = 0;
    virtual void onLoadError(std::function<void(int)> callback) = 0;
    
    virtual void* getNativeHandle() const = 0;
};

/**
 * @brief Document interface
 */
class Document {
public:
    virtual ~Document() = default;
    
    virtual std::string getTitle() const = 0;
    virtual std::string getURL() const = 0;
    virtual std::string getDocumentURI() const = 0;
    
    virtual Element* getElementById(const std::string& id) = 0;
    virtual std::vector<Element*> getElementsByTagName(const std::string& tag) = 0;
    virtual std::vector<Element*> getElementsByClassName(const std::string& className) = 0;
    virtual Element* querySelector(const std::string& selector) = 0;
    virtual std::vector<Element*> querySelectorAll(const std::string& selector) = 0;
    
    virtual Element* createElement(const std::string& tagName) = 0;
    virtual Element* createTextNode(const std::string& text) = 0;
    virtual Element* createComment(const std::string& text) = 0;
    
    virtual void addEventListener(const std::string& type, 
                                 std::function<void()> callback) = 0;
    virtual void removeEventListener(const std::string& type) = 0;
    
    virtual std::string getHTML() const = 0;
    virtual void setHTML(const std::string& html) = 0;
    
    virtual std::string getTextContent() const = 0;
    virtual void setTextContent(const std::string& text) = 0;
    
    virtual bool isReady() const = 0;
    virtual void waitForReady() = 0;
};

/**
 * @brief Element interface
 */
class Element {
public:
    virtual ~Element() = default;
    
    virtual std::string getTagName() const = 0;
    virtual std::string getId() const = 0;
    virtual void setId(const std::string& id) = 0;
    virtual std::string getClassName() const = 0;
    virtual void setClassName(const std::string& className) = 0;
    
    virtual std::string getAttribute(const std::string& name) const = 0;
    virtual void setAttribute(const std::string& name, const std::string& value) = 0;
    virtual bool hasAttribute(const std::string& name) const = 0;
    virtual void removeAttribute(const std::string& name) = 0;
    
    virtual std::string getStyle(const std::string& property) const = 0;
    virtual void setStyle(const std::string& property, const std::string& value) = 0;
    
    virtual std::string getTextContent() const = 0;
    virtual void setTextContent(const std::string& text) = 0;
    virtual std::string getInnerHTML() const = 0;
    virtual void setInnerHTML(const std::string& html) = 0;
    virtual std::string getOuterHTML() const = 0;
    
    virtual Element* getParent() const = 0;
    virtual std::vector<Element*> getChildren() const = 0;
    virtual Element* getFirstChild() const = 0;
    virtual Element* getLastChild() const = 0;
    
    virtual void appendChild(Element* child) = 0;
    virtual void removeChild(Element* child) = 0;
    virtual void insertBefore(Element* newChild, Element* refChild) = 0;
    virtual Element* cloneNode(bool deep = true) = 0;
    
    virtual void addEventListener(const std::string& type, 
                                 std::function<void()> callback) = 0;
    virtual void removeEventListener(const std::string& type) = 0;
    virtual void click() = 0;
    virtual void focus() = 0;
    virtual void blur() = 0;
    
    virtual void scrollIntoView() = 0;
    virtual void getBoundingRect(int& x, int& y, int& width, int& height) = 0;
};

/**
 * @brief Network request interface
 */
class NetworkRequest {
public:
    enum class Method {
        GET,
        POST,
        PUT,
        DELETE,
        HEAD,
        OPTIONS,
        PATCH
    };
    
    enum class Priority {
        LOW,
        NORMAL,
        HIGH,
        VERY_HIGH
    };
    
    virtual ~NetworkRequest() = default;
    
    virtual void setMethod(Method method) = 0;
    virtual Method getMethod() const = 0;
    
    virtual void setURL(const std::string& url) = 0;
    virtual std::string getURL() const = 0;
    
    virtual void setHeader(const std::string& key, const std::string& value) = 0;
    virtual std::string getHeader(const std::string& key) const = 0;
    virtual std::unordered_map<std::string, std::string> getHeaders() const = 0;
    
    virtual void setBody(const std::string& body) = 0;
    virtual std::string getBody() const = 0;
    
    virtual void setPriority(Priority priority) = 0;
    virtual Priority getPriority() const = 0;
    
    virtual void setTimeout(int timeoutMs) = 0;
    virtual int getTimeout() const = 0;
    
    virtual void setFollowRedirects(bool follow) = 0;
    virtual bool getFollowRedirects() const = 0;
    
    virtual void send(std::function<void(int, const std::string&)> callback) = 0;
    virtual void cancel() = 0;
    virtual bool isPending() const = 0;
    virtual bool isComplete() const = 0;
};

/**
 * @brief Storage interface
 */
class Storage {
public:
    virtual ~Storage() = default;
    
    virtual void setItem(const std::string& key, const std::string& value) = 0;
    virtual std::string getItem(const std::string& key) const = 0;
    virtual void removeItem(const std::string& key) = 0;
    virtual void clear() = 0;
    virtual size_t getLength() const = 0;
    virtual std::vector<std::string> getKeys() const = 0;
    virtual bool hasKey(const std::string& key) const = 0;
};

/**
 * @brief Cookie interface
 */
class Cookie {
public:
    virtual ~Cookie() = default;
    
    virtual void set(const std::string& name, 
                     const std::string& value,
                     const std::string& domain = "",
                     const std::string& path = "/",
                     int maxAge = -1,
                     bool secure = false,
                     bool httpOnly = false) = 0;
    
    virtual std::string get(const std::string& name) const = 0;
    virtual void remove(const std::string& name) = 0;
    virtual void clear() = 0;
    virtual std::unordered_map<std::string, std::string> getAll() const = 0;
};

/**
 * @brief Extension interface
 */
class Extension {
public:
    virtual ~Extension() = default;
    
    virtual std::string getId() const = 0;
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;
    
    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
    
    virtual void onLoad() = 0;
    virtual void onUnload() = 0;
    virtual void onMessage(const std::string& message) = 0;
};

// ============================================================================
// Factory Functions
// ============================================================================

/**
 * @brief Create a browser instance
 * @return Browser instance
 */
std::unique_ptr<IBrowser> createBrowser();

/**
 * @brief Get the global browser instance
 * @return Global browser instance
 */
IBrowser* getBrowser();

/**
 * @brief Initialize the BMS API
 * @return true if initialized successfully
 */
bool initializeAPI();

/**
 * @brief Shutdown the BMS API
 */
void shutdownAPI();

// ============================================================================
// Error Codes
// ============================================================================

enum class ErrorCode {
    SUCCESS = 0,
    FAILED = 1,
    INVALID_ARGUMENT = 2,
    NOT_FOUND = 3,
    ALREADY_EXISTS = 4,
    PERMISSION_DENIED = 5,
    NETWORK_ERROR = 6,
    TIMEOUT = 7,
    CANCELLED = 8,
    INTERNAL_ERROR = 9
};

/**
 * @brief Get error message for error code
 * @param code Error code
 * @return Error message
 */
std::string getErrorMessage(ErrorCode code);

} // namespace API
} // namespace BMS

#endif // BMS_API_H