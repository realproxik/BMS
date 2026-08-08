// bms_dom_engine.h - BMS DOM Engine with Chromium Integration
#ifndef BMS_DOM_ENGINE_H
#define BMS_DOM_ENGINE_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <queue>
#include <thread>
#include <future>
#include <chrono>

// Chromium includes (modified for BMS)
#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_v8.h"
#include "include/cef_parser.h"
#include "include/cef_render_handler.h"

namespace BMS {
namespace DOM {

// ============================================================================
// BMS Node System
// ============================================================================

enum class NodeType : uint8_t {
    ELEMENT = 1,
    ATTRIBUTE = 2,
    TEXT = 3,
    CDATA_SECTION = 4,
    PROCESSING_INSTRUCTION = 7,
    COMMENT = 8,
    DOCUMENT = 9,
    DOCUMENT_TYPE = 10,
    DOCUMENT_FRAGMENT = 11,
    SHADOW_ROOT = 12,
    CUSTOM_ELEMENT = 13
};

enum class DocumentPosition : uint16_t {
    DISCONNECTED = 0x01,
    PRECEDING = 0x02,
    FOLLOWING = 0x04,
    CONTAINS = 0x08,
    CONTAINED_BY = 0x10,
    IMPLEMENTATION_SPECIFIC = 0x20
};

// ============================================================================
// BMS Node Class
// ============================================================================

class BMSNode {
public:
    BMSNode(NodeType type, const std::string& name = "");
    virtual ~BMSNode();
    
    // Node identification
    NodeType getType() const { return type_; }
    const std::string& getNodeName() const { return nodeName_; }
    void setNodeName(const std::string& name) { nodeName_ = name; }
    
    const std::string& getNodeValue() const { return nodeValue_; }
    void setNodeValue(const std::string& value) { nodeValue_ = value; }
    
    const std::string& getNamespaceURI() const { return namespaceURI_; }
    void setNamespaceURI(const std::string& ns) { namespaceURI_ = ns; }
    
    const std::string& getPrefix() const { return prefix_; }
    void setPrefix(const std::string& prefix) { prefix_ = prefix; }
    
    const std::string& getLocalName() const { return localName_; }
    void setLocalName(const std::string& localName) { localName_ = localName; }
    
    // Tree navigation
    BMSNode* getParent() const { return parent_; }
    BMSNode* getFirstChild() const { return firstChild_; }
    BMSNode* getLastChild() const { return lastChild_; }
    BMSNode* getPreviousSibling() const { return previousSibling_; }
    BMSNode* getNextSibling() const { return nextSibling_; }
    BMSNode* getOwnerDocument() const;
    BMSNode* getRootNode() const;
    
    // Child management
    size_t getChildCount() const { return children_.size(); }
    BMSNode* getChildAt(size_t index) const;
    bool hasChildNodes() const { return !children_.empty(); }
    bool hasChildren() const { return hasChildNodes(); }
    
    // Tree manipulation
    BMSNode* appendChild(std::unique_ptr<BMSNode> child);
    BMSNode* insertBefore(std::unique_ptr<BMSNode> newChild, BMSNode* refChild);
    BMSNode* replaceChild(std::unique_ptr<BMSNode> newChild, BMSNode* oldChild);
    BMSNode* removeChild(BMSNode* child);
    void normalize();
    
    // Cloning
    std::unique_ptr<BMSNode> clone(bool deep = true) const;
    bool isEqual(const BMSNode* other) const;
    bool isSame(const BMSNode* other) const { return this == other; }
    
    // Text content
    std::string getTextContent() const;
    void setTextContent(const std::string& text);
    
    // HTML/XML serialization
    std::string getOuterHTML() const;
    std::string getInnerHTML() const;
    std::string toXML(bool pretty = true) const;
    
    // Comparison
    uint16_t compareDocumentPosition(const BMSNode* other) const;
    bool contains(const BMSNode* other) const;
    bool isAncestor(const BMSNode* other) const;
    bool isDescendant(const BMSNode* other) const;
    
    // BMS specific features
    virtual void render(void* context) const;
    virtual void update(double deltaTime);
    virtual void onEvent(const std::string& event, void* data);
    
    // Custom data
    void* getUserData() const { return userData_; }
    void setUserData(void* data) { userData_ = data; }
    void setCustomProperty(const std::string& key, const std::string& value);
    std::string getCustomProperty(const std::string& key) const;
    
    // BMS search integration
    void setSearchIndex(const std::string& index);
    std::string getSearchIndex() const;
    void addToSearchIndex(const std::string& content);
    void removeFromSearchIndex(const std::string& content);
    bool searchContent(const std::string& query) const;
    
    // Debugging
    std::string toString() const;
    void print(int depth = 0) const;
    void printTree() const;
    
    // Factory methods
    static std::unique_ptr<BMSNode> createElement(const std::string& tagName);
    static std::unique_ptr<BMSNode> createTextNode(const std::string& text);
    static std::unique_ptr<BMSNode> createComment(const std::string& text);
    static std::unique_ptr<BMSNode> createDocument();
    static std::unique_ptr<BMSNode> createDocumentFragment();
    static std::unique_ptr<BMSNode> createShadowRoot();
    static std::unique_ptr<BMSNode> createCustomElement(const std::string& name);
    
protected:
    NodeType type_;
    std::string nodeName_;
    std::string nodeValue_;
    std::string namespaceURI_;
    std::string prefix_;
    std::string localName_;
    std::string searchIndex_;
    
    BMSNode* parent_ = nullptr;
    std::vector<std::unique_ptr<BMSNode>> children_;
    BMSNode* firstChild_ = nullptr;
    BMSNode* lastChild_ = nullptr;
    BMSNode* previousSibling_ = nullptr;
    BMSNode* nextSibling_ = nullptr;
    BMSNode* ownerDocument_ = nullptr;
    
    void* userData_ = nullptr;
    std::unordered_map<std::string, std::string> customProperties_;
    
private:
    void updateChildPointers();
    void rebuildSearchIndex();
};

// ============================================================================
// BMS Element Class
// ============================================================================

class BMSElement : public BMSNode {
public:
    explicit BMSElement(const std::string& tagName);
    virtual ~BMSElement() = default;
    
    std::string getTagName() const { return nodeName_; }
    
    void setAttribute(const std::string& name, const std::string& value);
    std::string getAttribute(const std::string& name) const;
    bool hasAttribute(const std::string& name) const;
    void removeAttribute(const std::string& name);
    std::unordered_map<std::string, std::string> getAllAttributes() const;
    
    void setClassName(const std::string& className);
    std::string getClassName() const;
    bool hasClass(const std::string& className) const;
    void addClass(const std::string& className);
    void removeClass(const std::string& className);
    void toggleClass(const std::string& className);
    
    void setId(const std::string& id);
    std::string getId() const;
    
    void setStyle(const std::string& property, const std::string& value);
    std::string getStyle(const std::string& property) const;
    std::unordered_map<std::string, std::string> getAllStyles() const;
    
    void addEventListener(const std::string& event, std::function<void(void*)> callback);
    void removeEventListener(const std::string& event);
    void dispatchEvent(const std::string& event, void* data = nullptr);
    
    void render(void* context) const override;
    void update(double deltaTime) override;
    void onEvent(const std::string& event, void* data) override;
    
    // Shadow DOM support
    std::unique_ptr<BMSNode> attachShadow();
    BMSNode* getShadowRoot() const { return shadowRoot_.get(); }
    
    // Custom element support
    void connectedCallback();
    void disconnectedCallback();
    void adoptedCallback();
    void attributeChangedCallback(const std::string& name, 
                                 const std::string& oldValue, 
                                 const std::string& newValue);
    
private:
    std::unordered_map<std::string, std::string> attributes_;
    std::unordered_map<std::string, std::string> styles_;
    std::unordered_map<std::string, std::function<void(void*)>> eventListeners_;
    std::unique_ptr<BMSNode> shadowRoot_;
    bool isConnected_ = false;
};

// ============================================================================
// BMS Document Class
// ============================================================================

class BMSDocument : public BMSNode {
public:
    BMSDocument();
    virtual ~BMSDocument() = default;
    
    std::string getTitle() const;
    void setTitle(const std::string& title);
    std::string getURL() const { return url_; }
    void setURL(const std::string& url) { url_ = url; }
    
    BMSElement* getDocumentElement() const;
    BMSElement* getElementById(const std::string& id) const;
    std::vector<BMSElement*> getElementsByTagName(const std::string& tagName) const;
    std::vector<BMSElement*> getElementsByClassName(const std::string& className) const;
    BMSElement* querySelector(const std::string& selector) const;
    std::vector<BMSElement*> querySelectorAll(const std::string& selector) const;
    
    void parseHTML(const std::string& html);
    void parseXML(const std::string& xml);
    
    void render(void* context) const override;
    
    // Search integration
    void indexDocument();
    std::vector<BMSNode*> search(const std::string& query) const;
    std::vector<BMSNode*> searchGlobal(const std::string& query) const;
    void addToGlobalSearchIndex(const std::string& url, const std::string& content);
    void removeFromGlobalSearchIndex(const std::string& url);
    
private:
    std::string url_;
    std::string title_;
    std::unordered_map<std::string, std::string> globalSearchIndex_;
    mutable std::mutex searchMutex_;
    
    BMSElement* findElementById(BMSNode* node, const std::string& id) const;
    void findElementsByTagName(BMSNode* node, const std::string& tagName, 
                              std::vector<BMSElement*>& results) const;
    void findElementsByClassName(BMSNode* node, const std::string& className,
                                std::vector<BMSElement*>& results) const;
};

// ============================================================================
// BMS Search Engine
// ============================================================================

class BMSSearchEngine {
public:
    static BMSSearchEngine* getInstance();
    
    void indexPage(const std::string& url, const std::string& html);
    void indexPage(const std::string& url, const std::string& html, const std::string& title);
    void indexPage(const std::string& url, BMSDocument* document);
    
    std::vector<std::pair<std::string, float>> search(const std::string& query, 
                                                      int maxResults = 50);
    std::vector<std::pair<std::string, float>> searchGlobal(const std::string& query,
                                                            int maxResults = 100);
    
    void addWebsite(const std::string& url, const std::string& baseUrl);
    void removeWebsite(const std::string& url);
    void crawlWebsite(const std::string& url, int maxPages = 1000);
    
    void updateIndex(const std::string& url, const std::string& content);
    void removeFromIndex(const std::string& url);
    void rebuildIndex();
    
    std::vector<std::string> getRelatedPages(const std::string& url, int maxResults = 10);
    std::vector<std::string> getTopPages(int count = 10);
    
private:
    BMSSearchEngine() = default;
    ~BMSSearchEngine() = default;
    
    static BMSSearchEngine* instance_;
    
    struct SearchEntry {
        std::string url;
        std::string title;
        std::string content;
        float relevance = 0.0f;
        int visits = 0;
        std::chrono::system_clock::time_point lastVisited;
    };
    
    std::unordered_map<std::string, SearchEntry> index_;
    std::unordered_map<std::string, std::vector<std::string>> websiteLinks_;
    std::mutex mutex_;
    
    float calculateRelevance(const std::string& content, const std::string& query);
    void tokenize(const std::string& text, std::vector<std::string>& tokens);
    void stemWord(std::string& word);
    void normalizeText(std::string& text);
};

// ============================================================================
// BMS Browser Engine
// ============================================================================

class BMSBrowserEngine {
public:
    BMSBrowserEngine();
    ~BMSBrowserEngine();
    
    bool initialize(int argc, char** argv);
    void shutdown();
    
    void createWindow(const std::string& url, int width = 1024, int height = 768);
    void closeWindow(void* windowHandle);
    void navigate(void* windowHandle, const std::string& url);
    void refresh(void* windowHandle);
    void goBack(void* windowHandle);
    void goForward(void* windowHandle);
    
    // DOM access
    BMSDocument* getDocument(void* windowHandle) const;
    BMSElement* getElementById(void* windowHandle, const std::string& id) const;
    std::vector<BMSElement*> getElementsByTagName(void* windowHandle, const std::string& tagName) const;
    
    // Search
    void search(const std::string& query);
    void searchGlobal(const std::string& query);
    void addSearchEngine(const std::string& name, const std::string& url);
    void setDefaultSearchEngine(const std::string& name);
    std::vector<std::string> getSearchEngines() const;
    
    // BMS ignore system
    void loadIgnorePatterns(const std::string& filePath = ".bmsignore");
    bool shouldIgnore(const std::string& path) const;
    void addIgnorePattern(const std::string& pattern, bool isRegex = false);
    void removeIgnorePattern(const std::string& pattern);
    
    // API system
    void registerAPI(const std::string& name, std::function<void(const std::string&)> handler);
    void callAPI(const std::string& name, const std::string& data);
    void registerAPIModule(const std::string& moduleName, void* module);
    
    // Chromium integration
    CefRefPtr<CefBrowser> getBrowser(void* windowHandle) const;
    void injectJavaScript(void* windowHandle, const std::string& script);
    std::string evaluateJavaScript(void* windowHandle, const std::string& script);
    
    void onLoadStart(void* windowHandle);
    void onLoadEnd(void* windowHandle, int httpStatusCode);
    void onTitleChange(void* windowHandle, const std::string& title);
    void onAddressChange(void* windowHandle, const std::string& url);
    void onConsoleMessage(void* windowHandle, const std::string& message);
    
private:
    struct WindowContext {
        void* windowHandle;
        CefRefPtr<CefBrowser> browser;
        std::unique_ptr<BMSDocument> document;
        std::string currentUrl;
        std::string title;
        std::vector<std::string> history;
        int historyIndex = -1;
    };
    
    std::unordered_map<void*, std::unique_ptr<WindowContext>> windows_;
    std::unordered_map<std::string, std::function<void(const std::string&)>> apiHandlers_;
    std::unordered_map<std::string, void*> apiModules_;
    std::vector<std::string> ignorePatterns_;
    std::vector<bool> ignorePatternIsRegex_;
    std::mutex mutex_;
    bool initialized_ = false;
    
    std::unique_ptr<BMSSearchEngine> searchEngine_;
    
    // Chromium message handler
    class BMSBrowserClient : public CefClient {
    public:
        explicit BMSBrowserClient(BMSBrowserEngine* engine);
        // CefClient overrides
        // ...
    private:
        BMSBrowserEngine* engine_;
        IMPLEMENT_REFCOUNTING(BMSBrowserClient);
    };
};

// ============================================================================
// BMS Layout Engine
// ============================================================================

class BMSLayoutEngine {
public:
    BMSLayoutEngine();
    ~BMSLayoutEngine() = default;
    
    struct LayoutBox {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        BMSNode* node = nullptr;
        std::vector<LayoutBox> children;
        std::unordered_map<std::string, std::string> computedStyles;
    };
    
    LayoutBox computeLayout(BMSNode* root, int viewportWidth, int viewportHeight);
    void updateLayout(LayoutBox& box);
    void renderLayout(const LayoutBox& box, void* context);
    
    void setStyleResolver(std::function<std::unordered_map<std::string, std::string>(BMSElement*)> resolver);
    
private:
    void computeBoxModel(LayoutBox& box);
    void computePosition(LayoutBox& box, int parentX, int parentY);
    void computeFlexbox(LayoutBox& box);
    void computeGrid(LayoutBox& box);
    void computeBlockLayout(LayoutBox& box);
    void computeInlineLayout(LayoutBox& box);
    
    std::function<std::unordered_map<std::string, std::string>(BMSElement*)> styleResolver_;
};

} // namespace DOM
} // namespace BMS

#endif // BMS_DOM_ENGINE_H