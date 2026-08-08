// toolbar.h - Toolbar Header for BMS Browser
#ifndef BMS_TOOLBAR_H
#define BMS_TOOLBAR_H

#include "tool.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace BMS {
namespace UI {

// ============================================================================
// Toolbar Interface
// ============================================================================

/**
 * @brief Toolbar interface for browser UI
 */
class IToolbar {
public:
    virtual ~IToolbar() = default;
    
    // Toolbar management
    virtual void addTool(std::unique_ptr<ITool> tool) = 0;
    virtual void removeTool(const std::string& id) = 0;
    virtual void removeTool(int index) = 0;
    virtual void clearTools() = 0;
    virtual int getToolCount() const = 0;
    virtual ITool* getTool(int index) const = 0;
    virtual ITool* getTool(const std::string& id) const = 0;
    virtual std::vector<ITool*> getTools() const = 0;
    
    // Toolbar appearance
    virtual void setOrientation(ToolAlignment orientation) = 0;
    virtual ToolAlignment getOrientation() const = 0;
    virtual void setSpacing(int spacing) = 0;
    virtual int getSpacing() const = 0;
    virtual void setPadding(int padding) = 0;
    virtual int getPadding() const = 0;
    virtual void setBackgroundColor(const std::string& color) = 0;
    virtual std::string getBackgroundColor() const = 0;
    virtual void setBorderColor(const std::string& color) = 0;
    virtual std::string getBorderColor() const = 0;
    virtual void setBorderWidth(int width) = 0;
    virtual int getBorderWidth() const = 0;
    virtual void setCornerRadius(int radius) = 0;
    virtual int getCornerRadius() const = 0;
    
    // Toolbar behavior
    virtual void setAutoHide(bool autoHide) = 0;
    virtual bool getAutoHide() const = 0;
    virtual void setFloating(bool floating) = 0;
    virtual bool getFloating() const = 0;
    virtual void setResizable(bool resizable) = 0;
    virtual bool getResizable() const = 0;
    virtual void setMovable(bool movable) = 0;
    virtual bool getMovable() const = 0;
    
    // Events
    virtual void onToolAdded(std::function<void(ITool*)> callback) = 0;
    virtual void onToolRemoved(std::function<void(const std::string&)> callback) = 0;
    virtual void onToolClicked(std::function<void(ITool*)> callback) = 0;
    virtual void onToolbarShown(std::function<void()> callback) = 0;
    virtual void onToolbarHidden(std::function<void()> callback) = 0;
    
    // Rendering
    virtual void render(void* context) = 0;
    virtual void update() = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual bool isVisible() const = 0;
    
    // Position and size
    virtual void setPosition(int x, int y) = 0;
    virtual void getPosition(int& x, int& y) const = 0;
    virtual void setSize(int width, int height) = 0;
    virtual void getSize(int& width, int& height) const = 0;
    
    // Native handle
    virtual void* getNativeHandle() const = 0;
};

// ============================================================================
// Toolbar Factory
// ============================================================================

class ToolbarFactory {
public:
    static std::unique_ptr<IToolbar> createToolbar();
    static std::unique_ptr<IToolbar> createMainToolbar();
    static std::unique_ptr<IToolbar> createAddressToolbar();
    static std::unique_ptr<IToolbar> createBookmarkToolbar();
    static std::unique_ptr<IToolbar> createStatusToolbar();
};

} // namespace UI
} // namespace BMS

#endif // BMS_TOOLBAR_H