// tool.h - Base Tool Interface for BMS Browser
#ifndef BMS_TOOL_H
#define BMS_TOOL_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <cstdint>

namespace BMS {
namespace UI {

// ============================================================================
// Tool Types and Enumerations
// ============================================================================

enum class ToolType : uint8_t {
    BUTTON,
    TOGGLE,
    DROPDOWN,
    SLIDER,
    TEXT_INPUT,
    SEARCH_BAR,
    ADDRESS_BAR,
    MENU,
    SEPARATOR,
    CUSTOM
};

enum class ToolState : uint8_t {
    NORMAL,
    HOVER,
    ACTIVE,
    DISABLED,
    CHECKED,
    UNCHECKED
};

enum class ToolAlignment : uint8_t {
    LEFT,
    CENTER,
    RIGHT,
    TOP,
    BOTTOM
};

// ============================================================================
// Tool Event Types
// ============================================================================

struct ToolEvent {
    enum Type {
        CLICK,
        DOUBLE_CLICK,
        RIGHT_CLICK,
        HOVER,
        LEAVE,
        DRAG_START,
        DRAG_END,
        DROP,
        KEY_DOWN,
        KEY_UP,
        TEXT_CHANGED,
        VALUE_CHANGED,
        STATE_CHANGED
    };
    
    Type type;
    int x, y;
    int button;
    std::string text;
    std::string value;
    void* userData;
    
    ToolEvent(Type t = CLICK) : type(t), x(0), y(0), button(0), userData(nullptr) {}
};

// ============================================================================
// Tool Interface
// ============================================================================

class ITool {
public:
    virtual ~ITool() = default;
    
    // Identification
    virtual std::string getId() const = 0;
    virtual void setId(const std::string& id) = 0;
    virtual std::string getTooltip() const = 0;
    virtual void setTooltip(const std::string& tooltip) = 0;
    
    // State
    virtual ToolType getType() const = 0;
    virtual ToolState getState() const = 0;
    virtual void setState(ToolState state) = 0;
    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
    virtual bool isVisible() const = 0;
    virtual void setVisible(bool visible) = 0;
    
    // Position and Size
    virtual int getX() const = 0;
    virtual int getY() const = 0;
    virtual void setPosition(int x, int y) = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual void setSize(int width, int height) = 0;
    virtual void getBounds(int& x, int& y, int& width, int& height) const = 0;
    
    // Content
    virtual std::string getLabel() const = 0;
    virtual void setLabel(const std::string& label) = 0;
    virtual std::string getIcon() const = 0;
    virtual void setIcon(const std::string& iconPath) = 0;
    virtual std::string getValue() const = 0;
    virtual void setValue(const std::string& value) = 0;
    
    // Events
    virtual void addEventListener(ToolEvent::Type event, 
                                 std::function<void(const ToolEvent&)> callback) = 0;
    virtual void removeEventListener(ToolEvent::Type event) = 0;
    virtual void dispatchEvent(const ToolEvent& event) = 0;
    
    // Rendering
    virtual void render(void* context) = 0;
    virtual void update() = 0;
    
    // Parent/Child
    virtual void setParent(ITool* parent) = 0;
    virtual ITool* getParent() const = 0;
    virtual void addChild(std::unique_ptr<ITool> child) = 0;
    virtual void removeChild(ITool* child) = 0;
    virtual std::vector<ITool*> getChildren() const = 0;
    
    // Custom Data
    virtual void setUserData(void* data) = 0;
    virtual void* getUserData() const = 0;
    virtual void setCustomProperty(const std::string& key, const std::string& value) = 0;
    virtual std::string getCustomProperty(const std::string& key) const = 0;
};

// ============================================================================
// Tool Factory
// ============================================================================

class ToolFactory {
public:
    static std::unique_ptr<ITool> createTool(ToolType type);
    static std::unique_ptr<ITool> createButton(const std::string& label);
    static std::unique_ptr<ITool> createToggle(const std::string& label);
    static std::unique_ptr<ITool> createDropdown(const std::vector<std::string>& items);
    static std::unique_ptr<ITool> createSlider(int min, int max, int value);
    static std::unique_ptr<ITool> createTextInput(const std::string& placeholder);
    static std::unique_ptr<ITool> createSearchBar();
    static std::unique_ptr<ITool> createAddressBar();
    static std::unique_ptr<ITool> createMenu(const std::vector<std::string>& items);
    static std::unique_ptr<ITool> createSeparator();
};

} // namespace UI
} // namespace BMS

#endif // BMS_TOOL_H