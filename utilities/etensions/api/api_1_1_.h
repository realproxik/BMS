// api_1_1_.h - API v1.1 Header
#ifndef BMS_API_1_1_H
#define BMS_API_1_1_H

#include "api.h"

namespace BMS {
namespace API {
namespace v1_1 {

// ============================================================================
// Enhanced API for version 1.1
// ============================================================================

/**
 * @brief Enhanced browser interface with additional features
 */
class IBrowser_v1_1 : public IBrowser {
public:
    virtual ~IBrowser_v1_1() = default;
    
    // New features in v1.1
    virtual void setUserAgent(const std::string& userAgent) = 0;
    virtual std::string getUserAgent() const = 0;
    
    virtual void setProxy(const std::string& proxy) = 0;
    virtual std::string getProxy() const = 0;
    
    virtual void enableIncognitoMode(bool enabled) = 0;
    virtual bool isIncognitoMode() const = 0;
    
    virtual void clearCache() = 0;
    virtual void clearCookies() = 0;
    virtual void clearAllData() = 0;
    
    virtual void setHomepage(const std::string& url) = 0;
    virtual std::string getHomepage() const = 0;
    
    virtual void setDefaultSearchEngine(const std::string& engine) = 0;
    virtual std::string getDefaultSearchEngine() const = 0;
    
    virtual std::vector<std::string> getSearchEngines() const = 0;
    virtual void addSearchEngine(const std::string& name, const std::string& url) = 0;
    virtual void removeSearchEngine(const std::string& name) = 0;
};

/**
 * @brief Enhanced window interface
 */
class IWindow_v1_1 : public Window {
public:
    virtual ~IWindow_v1_1() = default;
    
    virtual void setOpacity(double opacity) = 0;
    virtual double getOpacity() const = 0;
    
    virtual void setAlwaysOnTop(bool onTop) = 0;
    virtual bool isAlwaysOnTop() const = 0;
    
    virtual void setResizable(bool resizable) = 0;
    virtual bool isResizable() const = 0;
    
    virtual void setMinimumSize(int width, int height) = 0;
    virtual void getMinimumSize(int& width, int& height) const = 0;
    
    virtual void setMaximumSize(int width, int height) = 0;
    virtual void getMaximumSize(int& width, int& height) const = 0;
};

/**
 * @brief Enhanced tab interface
 */
class ITab_v1_1 : public Tab {
public:
    virtual ~ITab_v1_1() = default;
    
    virtual void setUserAgent(const std::string& userAgent) = 0;
    virtual std::string getUserAgent() const = 0;
    
    virtual void setColor(const std::string& color) = 0;
    virtual std::string getColor() const = 0;
    
    virtual void setIcon(const std::string& iconPath) = 0;
    virtual std::string getIcon() const = 0;
    
    virtual void pinTab(bool pinned) = 0;
    virtual bool isPinned() const = 0;
    
    virtual void duplicate() = 0;
    virtual void close() = 0;
    
    virtual void saveAsPDF(const std::string& path) = 0;
    virtual void print() = 0;
    
    virtual void findText(const std::string& text) = 0;
    virtual void findAllText(const std::string& text) = 0;
    virtual void findNext() = 0;
    virtual void findPrevious() = 0;
};

/**
 * @brief Enhanced document interface
 */
class IDocument_v1_1 : public Document {
public:
    virtual ~IDocument_v1_1() = default;
    
    virtual std::string getCookies() const = 0;
    virtual void setCookie(const std::string& cookie) = 0;
    
    virtual std::string getReferrer() const = 0;
    virtual std::string getMimeType() const = 0;
    virtual std::string getCharacterSet() const = 0;
    
    virtual void setDesignMode(bool enabled) = 0;
    virtual bool isDesignMode() const = 0;
    
    virtual void setContentEditable(bool editable) = 0;
    virtual bool isContentEditable() const = 0;
    
    virtual std::string getSelection() const = 0;
    virtual void setSelection(const std::string& selection) = 0;
    virtual void selectAll() = 0;
    virtual void clearSelection() = 0;
    
    virtual void execCommand(const std::string& command) = 0;
    virtual void execCommand(const std::string& command, const std::string& value) = 0;
    virtual bool queryCommandEnabled(const std::string& command) const = 0;
    virtual bool queryCommandState(const std::string& command) const = 0;
};

// ============================================================================
// Factory Functions for v1.1
// ============================================================================

std::unique_ptr<IBrowser_v1_1> createBrowser_v1_1();

} // namespace v1_1
} // namespace API
} // namespace BMS

#endif // BMS_API_1_1_H