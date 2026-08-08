// api.cpp - Main API Implementation
#include "api.h"
#include <iostream>
#include <mutex>
#include <memory>
#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace BMS {
namespace API {

// ============================================================================
// Internal Implementation Classes
// ============================================================================

class BrowserImpl : public IBrowser {
public:
    BrowserImpl() : running_(false) {
        std::cout << "[BMS API] Browser instance created" << std::endl;
    }
    
    ~BrowserImpl() override {
        shutdown();
    }
    
    bool initialize(int argc, char** argv) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (running_) {
            std::cerr << "[BMS API] Browser already running" << std::endl;
            return false;
        }
        
        std::cout << "[BMS API] Initializing browser..." << std::endl;
        std::cout << "[BMS API] Arguments: " << argc << " arguments" << std::endl;
        
        // Initialize subsystems
        initializeSubsystems();
        
        running_ = true;
        std::cout << "[BMS API] Browser initialized successfully" << std::endl;
        
        return true;
    }
    
    void shutdown() override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!running_) return;
        
        std::cout << "[BMS API] Shutting down browser..." << std::endl;
        
        // Close all windows
        for (auto& window : windows_) {
            closeWindow(window);
        }
        windows_.clear();
        
        // Close all tabs
        for (auto& tab : tabs_) {
            closeTab(tab);
        }
        tabs_.clear();
        
        // Save settings
        saveSettings();
        
        running_ = false;
        std::cout << "[BMS API] Browser shut down successfully" << std::endl;
    }
    
    bool isRunning() const override {
        return running_;
    }
    
    Window* createWindow(const std::string& url, int width, int height) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::cout << "[BMS API] Creating window: " << url 
                  << " (" << width << "x" << height << ")" << std::endl;
        
        auto window = std::make_unique<WindowImpl>(this, url, width, height);
        Window* ptr = window.get();
        windows_.push_back(std::move(window));
        
        // Create default tab
        if (!url.empty()) {
            createTab(url, ptr);
        }
        
        return ptr;
    }
    
    void closeWindow(Window* window) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = std::find_if(windows_.begin(), windows_.end(),
            [window](const std::unique_ptr<WindowImpl>& w) {
                return w.get() == window;
            });
        
        if (it != windows_.end()) {
            std::cout << "[BMS API] Closing window" << std::endl;
            windows_.erase(it);
        }
    }
    
    std::vector<Window*> getWindows() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<Window*> result;
        for (const auto& window : windows_) {
            result.push_back(window.get());
        }
        return result;
    }
    
    Window* getActiveWindow() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return windows_.empty() ? nullptr : windows_.front().get();
    }
    
    Tab* createTab(const std::string& url, Window* window) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::cout << "[BMS API] Creating tab: " << url << std::endl;
        
        auto tab = std::make_unique<TabImpl>(this, url);
        Tab* ptr = tab.get();
        tabs_.push_back(std::move(tab));
        
        // Add to window if specified
        if (window) {
            window->addTab(ptr);
        }
        
        // Load URL if provided
        if (!url.empty()) {
            ptr->loadURL(url);
        }
        
        return ptr;
    }
    
    void closeTab(Tab* tab) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = std::find_if(tabs_.begin(), tabs_.end(),
            [tab](const std::unique_ptr<TabImpl>& t) {
                return t.get() == tab;
            });
        
        if (it != tabs_.end()) {
            std::cout << "[BMS API] Closing tab" << std::endl;
            tabs_.erase(it);
        }
    }
    
    std::vector<Tab*> getTabs() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<Tab*> result;
        for (const auto& tab : tabs_) {
            result.push_back(tab.get());
        }
        return result;
    }
    
    Tab* getActiveTab() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return tabs_.empty() ? nullptr : tabs_.front().get();
    }
    
    void navigate(const std::string& url, Tab* tab) override {
        if (!tab) {
            tab = getActiveTab();
        }
        
        if (tab) {
            tab->loadURL(url);
        } else {
            std::cerr << "[BMS API] No tab available for navigation" << std::endl;
        }
    }
    
    void goBack(Tab* tab) override {
        if (!tab) tab = getActiveTab();
        if (tab) tab->goBack();
    }
    
    void goForward(Tab* tab) override {
        if (!tab) tab = getActiveTab();
        if (tab) tab->goForward();
    }
    
    void refresh(Tab* tab) override {
        if (!tab) tab = getActiveTab();
        if (tab) tab->reload();
    }
    
    void stop(Tab* tab) override {
        if (!tab) tab = getActiveTab();
        if (tab) tab->stop();
    }
    
    void addToHistory(const std::string& url, const std::string& title) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        history_.push_back({url, title, std::time(nullptr)});
        
        // Keep history size manageable
        while (history_.size() > MAX_HISTORY) {
            history_.erase(history_.begin());
        }
        
        std::cout << "[BMS API] Added to history: " << title << " (" << url << ")" << std::endl;
    }
    
    std::vector<std::string> getHistory() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<std::string> result;
        for (const auto& entry : history_) {
            result.push_back(entry.url);
        }
        return result;
    }
    
    void clearHistory() override {
        std::lock_guard<std::mutex> lock(mutex_);
        history_.clear();
        std::cout << "[BMS API] History cleared" << std::endl;
    }
    
    void addBookmark(const std::string& url, const std::string& title) override {
        std::lock_guard<std::mutex> lock(mutex_);
        bookmarks_[url] = title;
        std::cout << "[BMS API] Bookmark added: " << title << " (" << url << ")" << std::endl;
    }
    
    void removeBookmark(const std::string& url) override {
        std::lock_guard<std::mutex> lock(mutex_);
        bookmarks_.erase(url);
        std::cout << "[BMS API] Bookmark removed: " << url << std::endl;
    }
    
    std::unordered_map<std::string, std::string> getBookmarks() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return bookmarks_;
    }
    
    bool installExtension(const std::string& path) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::cout << "[BMS API] Installing extension from: " << path << std::endl;
        
        // In a real implementation, this would load the extension
        auto ext = std::make_unique<ExtensionImpl>(path);
        ext->onLoad();
        extensions_.push_back(std::move(ext));
        
        return true;
    }
    
    void uninstallExtension(const std::string& id) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = std::find_if(extensions_.begin(), extensions_.end(),
            [&id](const std::unique_ptr<ExtensionImpl>& ext) {
                return ext->getId() == id;
            });
        
        if (it != extensions_.end()) {
            (*it)->onUnload();
            extensions_.erase(it);
            std::cout << "[BMS API] Extension uninstalled: " << id << std::endl;
        }
    }
    
    std::vector<Extension*> getExtensions() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<Extension*> result;
        for (const auto& ext : extensions_) {
            result.push_back(ext.get());
        }
        return result;
    }
    
    void setSetting(const std::string& key, const std::string& value) override {
        std::lock_guard<std::mutex> lock(mutex_);
        settings_[key] = value;
        std::cout << "[BMS API] Setting: " << key << " = " << value << std::endl;
    }
    
    std::string getSetting(const std::string& key) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = settings_.find(key);
        return it != settings_.end() ? it->second : "";
    }
    
    void saveSettings() override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::ofstream file("bms_settings.json");
        if (file.is_open()) {
            file << "{\n";
            for (auto it = settings_.begin(); it != settings_.end(); ++it) {
                file << "  \"" << it->first << "\": \"" << it->second << "\"";
                if (std::next(it) != settings_.end()) {
                    file << ",";
                }
                file << "\n";
            }
            file << "}\n";
            file.close();
            std::cout << "[BMS API] Settings saved" << std::endl;
        }
    }
    
    void loadSettings() override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::ifstream file("bms_settings.json");
        if (file.is_open()) {
            // Simple JSON parsing (in real implementation, use a proper JSON library)
            std::string line;
            while (std::getline(file, line)) {
                // Skip empty lines and braces
                if (line.empty() || line.find('{') != std::string::npos || 
                    line.find('}') != std::string::npos) {
                    continue;
                }
                
                // Parse key: value pairs
                size_t colonPos = line.find(':');
                if (colonPos != std::string::npos) {
                    std::string key = line.substr(0, colonPos);
                    std::string value = line.substr(colonPos + 1);
                    
                    // Clean up
                    key.erase(0, key.find_first_not_of(" \t\""));
                    key.erase(key.find_last_not_of(" \t\"") + 1);
                    value.erase(0, value.find_first_not_of(" \t\""));
                    value.erase(value.find_last_not_of(" \t\"") + 1);
                    
                    settings_[key] = value;
                }
            }
            file.close();
            std::cout << "[BMS API] Settings loaded: " << settings_.size() << " entries" << std::endl;
        }
    }
    
    void onPageLoad(std::function<void(const std::string&)> callback) override {
        std::lock_guard<std::mutex> lock(mutex_);
        pageLoadCallbacks_.push_back(callback);
    }
    
    void onTabOpen(std::function<void(Tab*)> callback) override {
        std::lock_guard<std::mutex> lock(mutex_);
        tabOpenCallbacks_.push_back(callback);
    }
    
    void onTabClose(std::function<void(Tab*)> callback) override {
        std::lock_guard<std::mutex> lock(mutex_);
        tabCloseCallbacks_.push_back(callback);
    }
    
    void onError(std::function<void(int, const std::string&)> callback) override {
        std::lock_guard<std::mutex> lock(mutex_);
        errorCallbacks_.push_back(callback);
    }
    
    // Internal methods for callbacks
    void triggerPageLoad(const std::string& url) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& callback : pageLoadCallbacks_) {
            callback(url);
        }
    }
    
    void triggerTabOpen(Tab* tab) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& callback : tabOpenCallbacks_) {
            callback(tab);
        }
    }
    
    void triggerTabClose(Tab* tab) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& callback : tabCloseCallbacks_) {
            callback(tab);
        }
    }
    
    void triggerError(int code, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& callback : errorCallbacks_) {
            callback(code, message);
        }
    }
    
private:
    void initializeSubsystems() {
        // Initialize network subsystem
        std::cout << "[BMS API] Initializing network subsystem..." << std::endl;
        
        // Initialize storage subsystem
        std::cout << "[BMS API] Initializing storage subsystem..." << std::endl;
        
        // Initialize UI subsystem
        std::cout << "[BMS API] Initializing UI subsystem..." << std::endl;
        
        // Load settings
        loadSettings();
    }
    
    // Inner class declarations
    class WindowImpl;
    class TabImpl;
    class DocumentImpl;
    class ElementImpl;
    class ExtensionImpl;
    
    struct HistoryEntry {
        std::string url;
        std::string title;
        std::time_t timestamp;
    };
    
    static constexpr size_t MAX_HISTORY = 1000;
    
    bool running_;
    mutable std::mutex mutex_;
    
    std::vector<std::unique_ptr<WindowImpl>> windows_;
    std::vector<std::unique_ptr<TabImpl>> tabs_;
    std::vector<std::unique_ptr<ExtensionImpl>> extensions_;
    
    std::vector<HistoryEntry> history_;
    std::unordered_map<std::string, std::string> bookmarks_;
    std::unordered_map<std::string, std::string> settings_;
    
    std::vector<std::function<void(const std::string&)>> pageLoadCallbacks_;
    std::vector<std::function<void(Tab*)>> tabOpenCallbacks_;
    std::vector<std::function<void(Tab*)>> tabCloseCallbacks_;
    std::vector<std::function<void(int, const std::string&)>> errorCallbacks_;
};

// ============================================================================
// Window Implementation
// ============================================================================

class BrowserImpl::WindowImpl : public Window {
public:
    WindowImpl(BrowserImpl* browser, const std::string& url, int width, int height)
        : browser_(browser), width_(width), height_(height), 
          title_("BMS Browser"), visible_(false), maximized_(false),
          minimized_(false), fullscreen_(false) {
        std::cout << "[BMS API] Window created: " << width << "x" << height << std::endl;
    }
    
    ~WindowImpl() override {
        std::cout << "[BMS API] Window destroyed" << std::endl;
    }
    
    void show() override {
        visible_ = true;
        std::cout << "[BMS API] Window shown" << std::endl;
    }
    
    void hide() override {
        visible_ = false;
        std::cout << "[BMS API] Window hidden" << std::endl;
    }
    
    void maximize() override {
        maximized_ = true;
        minimized_ = false;
        std::cout << "[BMS API] Window maximized" << std::endl;
    }
    
    void minimize() override {
        minimized_ = true;
        maximized_ = false;
        std::cout << "[BMS API] Window minimized" << std::endl;
    }
    
    void restore() override {
        maximized_ = false;
        minimized_ = false;
        std::cout << "[BMS API] Window restored" << std::endl;
    }
    
    bool isVisible() const override { return visible_; }
    bool isMaximized() const override { return maximized_; }
    bool isMinimized() const override { return minimized_; }
    
    void setTitle(const std::string& title) override {
        title_ = title;
        std::cout << "[BMS API] Window title: " << title << std::endl;
    }
    
    std::string getTitle() const override { return title_; }
    
    void setSize(int width, int height) override {
        width_ = width;
        height_ = height;
        std::cout << "[BMS API] Window size: " << width << "x" << height << std::endl;
    }
    
    void getSize(int& width, int& height) const override {
        width = width_;
        height = height_;
    }
    
    void setPosition(int x, int y) override {
        x_ = x;
        y_ = y;
        std::cout << "[BMS API] Window position: " << x << "," << y << std::endl;
    }
    
    void getPosition(int& x, int& y) const override {
        x = x_;
        y = y_;
    }
    
    void setFullscreen(bool fullscreen) override {
        fullscreen_ = fullscreen;
        std::cout << "[BMS API] Fullscreen: " << (fullscreen ? "ON" : "OFF") << std::endl;
    }
    
    bool isFullscreen() const override { return fullscreen_; }
    
    void addTab(Tab* tab) override {
        tabs_.push_back(tab);
        if (tabs_.size() == 1) {
            activeTab_ = tab;
        }
        std::cout << "[BMS API] Tab added to window, total: " << tabs_.size() << std::endl;
    }
    
    void removeTab(Tab* tab) override {
        auto it = std::find(tabs_.begin(), tabs_.end(), tab);
        if (it != tabs_.end()) {
            tabs_.erase(it);
            if (activeTab_ == tab) {
                activeTab_ = tabs_.empty() ? nullptr : tabs_.front();
            }
            std::cout << "[BMS API] Tab removed, remaining: " << tabs_.size() << std::endl;
        }
    }
    
    std::vector<Tab*> getTabs() const override { return tabs_; }
    
    Tab* getActiveTab() const override { return activeTab_; }
    
    void setActiveTab(Tab* tab) override {
        if (std::find(tabs_.begin(), tabs_.end(), tab) != tabs_.end()) {
            activeTab_ = tab;
            std::cout << "[BMS API] Active tab changed" << std::endl;
        }
    }
    
    void* getNativeHandle() const override {
        // Return platform-specific window handle
        return nullptr;
    }
    
private:
    BrowserImpl* browser_;
    int width_, height_;
    int x_ = 0, y_ = 0;
    std::string title_;
    bool visible_, maximized_, minimized_, fullscreen_;
    std::vector<Tab*> tabs_;
    Tab* activeTab_ = nullptr;
};

// ============================================================================
// Tab Implementation
// ============================================================================

class BrowserImpl::TabImpl : public Tab {
public:
    TabImpl(BrowserImpl* browser, const std::string& url)
        : browser_(browser), url_(url), title_("New Tab"),
          loading_(false), progress_(0.0), zoomLevel_(1.0),
          muted_(false), canGoBack_(false), canGoForward_(false) {
        std::cout << "[BMS API] Tab created: " << url << std::endl;
        
        // Create document
        document_ = std::make_unique<DocumentImpl>(this);
    }
    
    ~TabImpl() override {
        std::cout << "[BMS API] Tab destroyed" << std::endl;
    }
    
    void loadURL(const std::string& url) override {
        url_ = url;
        loading_ = true;
        progress_ = 0.0;
        
        std::cout << "[BMS API] Loading URL: " << url << std::endl;
        
        // Simulate loading
        std::thread([this]() {
            // Simulate network request
            for (int i = 0; i <= 10; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                progress_ = i / 10.0;
                if (i == 10) {
                    loading_ = false;
                    title_ = "Loaded: " + url_;
                    document_->setHTML("<html><body><h1>BMS Browser</h1><p>Loaded: " + url_ + "</p></body></html>");
                    browser_->triggerPageLoad(url_);
                }
            }
        }).detach();
    }
    
    std::string getURL() const override { return url_; }
    std::string getTitle() const override { return title_; }
    
    void reload() override {
        if (!url_.empty()) {
            loadURL(url_);
        }
    }
    
    void stop() override {
        loading_ = false;
        std::cout << "[BMS API] Tab stopped loading" << std::endl;
    }
    
    void goBack() override {
        if (canGoBack_) {
            std::cout << "[BMS API] Going back" << std::endl;
            // In real implementation, navigate to previous URL
        }
    }
    
    void goForward() override {
        if (canGoForward_) {
            std::cout << "[BMS API] Going forward" << std::endl;
            // In real implementation, navigate to next URL
        }
    }
    
    bool canGoBack() const override { return canGoBack_; }
    bool canGoForward() const override { return canGoForward_; }
    
    Document* getDocument() const override {
        return document_.get();
    }
    
    void executeScript(const std::string& script) override {
        std::cout << "[BMS API] Executing script: " << script << std::endl;
        // In real implementation, execute JavaScript
    }
    
    std::string evaluateScript(const std::string& script) override {
        std::cout << "[BMS API] Evaluating script: " << script << std::endl;
        return "Script result (simulated)";
    }
    
    void setZoomLevel(double level) override {
        zoomLevel_ = std::max(0.25, std::min(5.0, level));
        std::cout << "[BMS API] Zoom level: " << zoomLevel_ << std::endl;
    }
    
    double getZoomLevel() const override { return zoomLevel_; }
    
    void setMuted(bool muted) override {
        muted_ = muted;
        std::cout << "[BMS API] Muted: " << (muted ? "ON" : "OFF") << std::endl;
    }
    
    bool isMuted() const override { return muted_; }
    bool isLoading() const override { return loading_; }
    double getLoadProgress() const override { return progress_; }
    
    void onLoadStart(std::function<void()> callback) override {
        loadStartCallbacks_.push_back(callback);
    }
    
    void onLoadFinish(std::function<void()> callback) override {
        loadFinishCallbacks_.push_back(callback);
    }
    
    void onLoadError(std::function<void(int)> callback) override {
        loadErrorCallbacks_.push_back(callback);
    }
    
    void* getNativeHandle() const override {
        return nullptr;
    }
    
private:
    class DocumentImpl : public Document {
    public:
        DocumentImpl(TabImpl* tab) : tab_(tab), ready_(true) {}
        
        std::string getTitle() const override {
            return tab_->getTitle();
        }
        
        std::string getURL() const override {
            return tab_->getURL();
        }
        
        std::string getDocumentURI() const override {
            return tab_->getURL();
        }
        
        Element* getElementById(const std::string& id) override {
            // In real implementation, find element by ID
            return nullptr;
        }
        
        std::vector<Element*> getElementsByTagName(const std::string& tag) override {
            return {};
        }
        
        std::vector<Element*> getElementsByClassName(const std::string& className) override {
            return {};
        }
        
        Element* querySelector(const std::string& selector) override {
            return nullptr;
        }
        
        std::vector<Element*> querySelectorAll(const std::string& selector) override {
            return {};
        }
        
        Element* createElement(const std::string& tagName) override {
            return new ElementImpl();
        }
        
        Element* createTextNode(const std::string& text) override {
            return new ElementImpl();
        }
        
        Element* createComment(const std::string& text) override {
            return new ElementImpl();
        }
        
        void addEventListener(const std::string& type, std::function<void()> callback) override {}
        void removeEventListener(const std::string& type) override {}
        
        std::string getHTML() const override { return html_; }
        void setHTML(const std::string& html) override { html_ = html; }
        
        std::string getTextContent() const override { return html_; }
        void setTextContent(const std::string& text) override { html_ = text; }
        
        bool isReady() const override { return ready_; }
        void waitForReady() override {}
        
    private:
        class ElementImpl : public Element {
        public:
            std::string getTagName() const override { return "div"; }
            std::string getId() const override { return ""; }
            void setId(const std::string& id) override {}
            std::string getClassName() const override { return ""; }
            void setClassName(const std::string& className) override {}
            
            std::string getAttribute(const std::string& name) const override { return ""; }
            void setAttribute(const std::string& name, const std::string& value) override {}
            bool hasAttribute(const std::string& name) const override { return false; }
            void removeAttribute(const std::string& name) override {}
            
            std::string getStyle(const std::string& property) const override { return ""; }
            void setStyle(const std::string& property, const std::string& value) override {}
            
            std::string getTextContent() const override { return ""; }
            void setTextContent(const std::string& text) override {}
            std::string getInnerHTML() const override { return ""; }
            void setInnerHTML(const std::string& html) override {}
            std::string getOuterHTML() const override { return ""; }
            
            Element* getParent() const override { return nullptr; }
            std::vector<Element*> getChildren() const override { return {}; }
            Element* getFirstChild() const override { return nullptr; }
            Element* getLastChild() const override { return nullptr; }
            
            void appendChild(Element* child) override {}
            void removeChild(Element* child) override {}
            void insertBefore(Element* newChild, Element* refChild) override {}
            Element* cloneNode(bool deep = true) override { return new ElementImpl(); }
            
            void addEventListener(const std::string& type, std::function<void()> callback) override {}
            void removeEventListener(const std::string& type) override {}
            void click() override {}
            void focus() override {}
            void blur() override {}
            
            void scrollIntoView() override {}
            void getBoundingRect(int& x, int& y, int& width, int& height) override {
                x = y = width = height = 0;
            }
        };
        
        TabImpl* tab_;
        std::string html_;
        bool ready_;
        std::vector<std::unique_ptr<Element>> elements_;
    };
    
    BrowserImpl* browser_;
    std::string url_;
    std::string title_;
    bool loading_;
    double progress_;
    double zoomLevel_;
    bool muted_;
    bool canGoBack_;
    bool canGoForward_;
    std::unique_ptr<DocumentImpl> document_;
    
    std::vector<std::function<void()>> loadStartCallbacks_;
    std::vector<std::function<void()>> loadFinishCallbacks_;
    std::vector<std::function<void(int)>> loadErrorCallbacks_;
};

// ============================================================================
// Extension Implementation
// ============================================================================

class BrowserImpl::ExtensionImpl : public Extension {
public:
    ExtensionImpl(const std::string& path) : path_(path), enabled_(true) {
        id_ = generateId(path);
        name_ = "Extension from: " + path;
        version_ = "1.0.0";
        description_ = "BMS Browser Extension";
    }
    
    std::string getId() const override { return id_; }
    std::string getName() const override { return name_; }
    std::string getVersion() const override { return version_; }
    std::string getDescription() const override { return description_; }
    
    bool isEnabled() const override { return enabled_; }
    void setEnabled(bool enabled) override { enabled_ = enabled; }
    
    void onLoad() override {
        std::cout << "[BMS API] Extension loaded: " << name_ << std::endl;
    }
    
    void onUnload() override {
        std::cout << "[BMS API] Extension unloaded: " << name_ << std::endl;
    }
    
    void onMessage(const std::string& message) override {
        std::cout << "[BMS API] Extension message: " << message << std::endl;
    }
    
private:
    std::string generateId(const std::string& path) {
        // Simple hash-based ID generation
        size_t hash = std::hash<std::string>{}(path);
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(8) << hash;
        return ss.str();
    }
    
    std::string id_;
    std::string name_;
    std::string version_;
    std::string description_;
    std::string path_;
    bool enabled_;
};

// ============================================================================
// Global Functions
// ============================================================================

std::unique_ptr<IBrowser> createBrowser() {
    return std::make_unique<BrowserImpl>();
}

IBrowser* getBrowser() {
    static std::unique_ptr<BrowserImpl> browser = nullptr;
    if (!browser) {
        browser = std::make_unique<BrowserImpl>();
    }
    return browser.get();
}

bool initializeAPI() {
    std::cout << "[BMS API] Initializing API..." << std::endl;
    
    auto* browser = getBrowser();
    if (!browser) {
        return false;
    }
    
    // Create dummy arguments if needed
    int argc = 0;
    char** argv = nullptr;
    char dummy[] = "bms_browser";
    argv = &dummy;
    argc = 1;
    
    return browser->initialize(argc, argv);
}

void shutdownAPI() {
    std::cout << "[BMS API] Shutting down API..." << std::endl;
    
    auto* browser = getBrowser();
    if (browser) {
        browser->shutdown();
    }
}

std::string getErrorMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS:
            return "Success";
        case ErrorCode::FAILED:
            return "Operation failed";
        case ErrorCode::INVALID_ARGUMENT:
            return "Invalid argument";
        case ErrorCode::NOT_FOUND:
            return "Not found";
        case ErrorCode::ALREADY_EXISTS:
            return "Already exists";
        case ErrorCode::PERMISSION_DENIED:
            return "Permission denied";
        case ErrorCode::NETWORK_ERROR:
            return "Network error";
        case ErrorCode::TIMEOUT:
            return "Timeout";
        case ErrorCode::CANCELLED:
            return "Cancelled";
        case ErrorCode::INTERNAL_ERROR:
            return "Internal error";
        default:
            return "Unknown error";
    }
}

} // namespace API
} // namespace BMS