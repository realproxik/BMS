// toolbar.cc - Toolbar Implementation (C++)
#include "toolbar.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace BMS {
namespace UI {

// ============================================================================
// Toolbar Implementation
// ============================================================================

class ToolbarImpl : public IToolbar {
public:
    ToolbarImpl() 
        : orientation_(ToolAlignment::LEFT),
          spacing_(4),
          padding_(4),
          borderWidth_(1),
          cornerRadius_(4),
          backgroundColor_("#f0f0f0"),
          borderColor_("#cccccc"),
          autoHide_(false),
          floating_(false),
          resizable_(false),
          movable_(false),
          visible_(true),
          x_(0),
          y_(0),
          width_(100),
          height_(32) {
        std::cout << "[BMS Toolbar] Created toolbar" << std::endl;
    }
    
    ~ToolbarImpl() override {
        std::cout << "[BMS Toolbar] Destroyed toolbar" << std::endl;
    }
    
    // ========================================================================
    // Tool management
    // ========================================================================
    
    void addTool(std::unique_ptr<ITool> tool) override {
        if (!tool) return;
        
        std::string id = tool->getId();
        if (id.empty()) {
            id = "tool_" + std::to_string(tools_.size());
            tool->setId(id);
        }
        
        // Check for duplicate ID
        if (findTool(id)) {
            std::cerr << "[BMS Toolbar] Tool with ID '" << id << "' already exists" << std::endl;
            return;
        }
        
        ITool* ptr = tool.get();
        tools_.push_back(std::move(tool));
        
        if (toolAddedCallback_) {
            toolAddedCallback_(ptr);
        }
        
        std::cout << "[BMS Toolbar] Added tool: " << id << std::endl;
        updateLayout();
    }
    
    void removeTool(const std::string& id) override {
        auto it = std::find_if(tools_.begin(), tools_.end(),
            [&id](const std::unique_ptr<ITool>& tool) {
                return tool->getId() == id;
            });
        
        if (it != tools_.end()) {
            std::string removedId = (*it)->getId();
            tools_.erase(it);
            
            if (toolRemovedCallback_) {
                toolRemovedCallback_(removedId);
            }
            
            std::cout << "[BMS Toolbar] Removed tool: " << removedId << std::endl;
            updateLayout();
        }
    }
    
    void removeTool(int index) override {
        if (index < 0 || index >= static_cast<int>(tools_.size())) {
            return;
        }
        
        std::string id = tools_[index]->getId();
        tools_.erase(tools_.begin() + index);
        
        if (toolRemovedCallback_) {
            toolRemovedCallback_(id);
        }
        
        updateLayout();
    }
    
    void clearTools() override {
        tools_.clear();
        updateLayout();
        std::cout << "[BMS Toolbar] Cleared all tools" << std::endl;
    }
    
    int getToolCount() const override {
        return static_cast<int>(tools_.size());
    }
    
    ITool* getTool(int index) const override {
        if (index < 0 || index >= static_cast<int>(tools_.size())) {
            return nullptr;
        }
        return tools_[index].get();
    }
    
    ITool* getTool(const std::string& id) const override {
        auto it = std::find_if(tools_.begin(), tools_.end(),
            [&id](const std::unique_ptr<ITool>& tool) {
                return tool->getId() == id;
            });
        
        return it != tools_.end() ? it->get() : nullptr;
    }
    
    std::vector<ITool*> getTools() const override {
        std::vector<ITool*> result;
        result.reserve(tools_.size());
        for (const auto& tool : tools_) {
            result.push_back(tool.get());
        }
        return result;
    }
    
    // ========================================================================
    // Toolbar appearance
    // ========================================================================
    
    void setOrientation(ToolAlignment orientation) override {
        orientation_ = orientation;
        updateLayout();
    }
    
    ToolAlignment getOrientation() const override {
        return orientation_;
    }
    
    void setSpacing(int spacing) override {
        spacing_ = std::max(0, spacing);
        updateLayout();
    }
    
    int getSpacing() const override {
        return spacing_;
    }
    
    void setPadding(int padding) override {
        padding_ = std::max(0, padding);
        updateLayout();
    }
    
    int getPadding() const override {
        return padding_;
    }
    
    void setBackgroundColor(const std::string& color) override {
        backgroundColor_ = color;
    }
    
    std::string getBackgroundColor() const override {
        return backgroundColor_;
    }
    
    void setBorderColor(const std::string& color) override {
        borderColor_ = color;
    }
    
    std::string getBorderColor() const override {
        return borderColor_;
    }
    
    void setBorderWidth(int width) override {
        borderWidth_ = std::max(0, width);
    }
    
    int getBorderWidth() const override {
        return borderWidth_;
    }
    
    void setCornerRadius(int radius) override {
        cornerRadius_ = std::max(0, radius);
    }
    
    int getCornerRadius() const override {
        return cornerRadius_;
    }
    
    // ========================================================================
    // Toolbar behavior
    // ========================================================================
    
    void setAutoHide(bool autoHide) override {
        autoHide_ = autoHide;
    }
    
    bool getAutoHide() const override {
        return autoHide_;
    }
    
    void setFloating(bool floating) override {
        floating_ = floating;
    }
    
    bool getFloating() const override {
        return floating_;
    }
    
    void setResizable(bool resizable) override {
        resizable_ = resizable;
    }
    
    bool getResizable() const override {
        return resizable_;
    }
    
    void setMovable(bool movable) override {
        movable_ = movable;
    }
    
    bool getMovable() const override {
        return movable_;
    }
    
    // ========================================================================
    // Events
    // ========================================================================
    
    void onToolAdded(std::function<void(ITool*)> callback) override {
        toolAddedCallback_ = callback;
    }
    
    void onToolRemoved(std::function<void(const std::string&)> callback) override {
        toolRemovedCallback_ = callback;
    }
    
    void onToolClicked(std::function<void(ITool*)> callback) override {
        toolClickedCallback_ = callback;
    }
    
    void onToolbarShown(std::function<void()> callback) override {
        toolbarShownCallback_ = callback;
    }
    
    void onToolbarHidden(std::function<void()> callback) override {
        toolbarHiddenCallback_ = callback;
    }
    
    // ========================================================================
    // Rendering
    // ========================================================================
    
    void render(void* context) override {
        if (!visible_) return;
        
        // Platform-specific rendering would go here
        std::cout << "[BMS Toolbar] Rendering toolbar with " 
                  << tools_.size() << " tools" << std::endl;
        
        for (const auto& tool : tools_) {
            if (tool->isVisible()) {
                tool->render(context);
            }
        }
    }
    
    void update() override {
        for (const auto& tool : tools_) {
            tool->update();
        }
    }
    
    void show() override {
        if (!visible_) {
            visible_ = true;
            if (toolbarShownCallback_) {
                toolbarShownCallback_();
            }
        }
    }
    
    void hide() override {
        if (visible_) {
            visible_ = false;
            if (toolbarHiddenCallback_) {
                toolbarHiddenCallback_();
            }
        }
    }
    
    bool isVisible() const override {
        return visible_;
    }
    
    // ========================================================================
    // Position and size
    // ========================================================================
    
    void setPosition(int x, int y) override {
        x_ = x;
        y_ = y;
        updateLayout();
    }
    
    void getPosition(int& x, int& y) const override {
        x = x_;
        y = y_;
    }
    
    void setSize(int width, int height) override {
        width_ = width;
        height_ = height;
        updateLayout();
    }
    
    void getSize(int& width, int& height) const override {
        width = width_;
        height = height_;
    }
    
    void* getNativeHandle() const override {
        // Return platform-specific handle
        return nullptr;
    }
    
    // ========================================================================
    // Layout
    // ========================================================================
    
    void updateLayout() {
        if (tools_.empty()) return;
        
        int currentX = x_ + padding_;
        int currentY = y_ + padding_;
        bool horizontal = (orientation_ == ToolAlignment::LEFT || 
                          orientation_ == ToolAlignment::RIGHT);
        
        for (const auto& tool : tools_) {
            if (!tool->isVisible()) continue;
            
            if (horizontal) {
                tool->setPosition(currentX, currentY);
                currentX += tool->getWidth() + spacing_;
            } else {
                tool->setPosition(currentX, currentY);
                currentY += tool->getHeight() + spacing_;
            }
        }
        
        // Update toolbar size based on children
        if (horizontal) {
            width_ = currentX - x_ + padding_;
            height_ = 0;
            for (const auto& tool : tools_) {
                if (tool->isVisible()) {
                    height_ = std::max(height_, tool->getHeight() + padding_ * 2);
                }
            }
        } else {
            height_ = currentY - y_ + padding_;
            width_ = 0;
            for (const auto& tool : tools_) {
                if (tool->isVisible()) {
                    width_ = std::max(width_, tool->getWidth() + padding_ * 2);
                }
            }
        }
    }
    
    // ========================================================================
    // Helper methods
    // ========================================================================
    
    ITool* findTool(const std::string& id) const {
        for (const auto& tool : tools_) {
            if (tool->getId() == id) {
                return tool.get();
            }
        }
        return nullptr;
    }

private:
    // Tools
    std::vector<std::unique_ptr<ITool>> tools_;
    
    // Appearance
    ToolAlignment orientation_;
    int spacing_;
    int padding_;
    int borderWidth_;
    int cornerRadius_;
    std::string backgroundColor_;
    std::string borderColor_;
    
    // Behavior
    bool autoHide_;
    bool floating_;
    bool resizable_;
    bool movable_;
    bool visible_;
    
    // Position
    int x_, y_;
    int width_, height_;
    
    // Callbacks
    std::function<void(ITool*)> toolAddedCallback_;
    std::function<void(const std::string&)> toolRemovedCallback_;
    std::function<void(ITool*)> toolClickedCallback_;
    std::function<void()> toolbarShownCallback_;
    std::function<void()> toolbarHiddenCallback_;
};

// ============================================================================
// Toolbar Factory Implementation
// ============================================================================

std::unique_ptr<IToolbar> ToolbarFactory::createToolbar() {
    return std::make_unique<ToolbarImpl>();
}

std::unique_ptr<IToolbar> ToolbarFactory::createMainToolbar() {
    auto toolbar = std::make_unique<ToolbarImpl>();
    toolbar->setOrientation(ToolAlignment::LEFT);
    toolbar->setSpacing(2);
    toolbar->setPadding(4);
    toolbar->setBackgroundColor("#f8f9fa");
    toolbar->setBorderColor("#dee2e6");
    toolbar->setBorderWidth(1);
    return toolbar;
}

std::unique_ptr<IToolbar> ToolbarFactory::createAddressToolbar() {
    auto toolbar = std::make_unique<ToolbarImpl>();
    toolbar->setOrientation(ToolAlignment::LEFT);
    toolbar->setSpacing(4);
    toolbar->setPadding(4);
    toolbar->setBackgroundColor("#ffffff");
    toolbar->setBorderColor("#e9ecef");
    toolbar->setBorderWidth(1);
    toolbar->setCornerRadius(8);
    return toolbar;
}

std::unique_ptr<IToolbar> ToolbarFactory::createBookmarkToolbar() {
    auto toolbar = std::make_unique<ToolbarImpl>();
    toolbar->setOrientation(ToolAlignment::LEFT);
    toolbar->setSpacing(0);
    toolbar->setPadding(2);
    toolbar->setBackgroundColor("#f8f9fa");
    toolbar->setBorderColor("#e9ecef");
    toolbar->setBorderWidth(0);
    return toolbar;
}

std::unique_ptr<IToolbar> ToolbarFactory::createStatusToolbar() {
    auto toolbar = std::make_unique<ToolbarImpl>();
    toolbar->setOrientation(ToolAlignment::LEFT);
    toolbar->setSpacing(10);
    toolbar->setPadding(4);
    toolbar->setBackgroundColor("#f8f9fa");
    toolbar->setBorderColor("#dee2e6");
    toolbar->setBorderWidth(1);
    return toolbar;
}

} // namespace UI
} // namespace BMS