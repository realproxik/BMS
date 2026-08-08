# tool.py - Python Tool Module
"""
BMS Browser - Tool Module
Provides GUI tool and widget functionality
"""

from enum import IntEnum
from typing import Optional, List, Dict, Callable, Any
from dataclasses import dataclass
import json

# ============================================================================
# Tool Types
# ============================================================================

class ToolType(IntEnum):
    """Tool type enumeration"""
    BUTTON = 0
    TOGGLE = 1
    DROPDOWN = 2
    SLIDER = 3
    TEXT_INPUT = 4
    SEARCH_BAR = 5
    ADDRESS_BAR = 6
    MENU = 7
    SEPARATOR = 8
    CUSTOM = 9

class ToolState(IntEnum):
    """Tool state enumeration"""
    NORMAL = 0
    HOVER = 1
    ACTIVE = 2
    DISABLED = 3
    CHECKED = 4
    UNCHECKED = 5

class ToolEvent(IntEnum):
    """Tool event types"""
    CLICK = 0
    DOUBLE_CLICK = 1
    RIGHT_CLICK = 2
    HOVER = 3
    LEAVE = 4
    DRAG_START = 5
    DRAG_END = 6
    DROP = 7
    KEY_DOWN = 8
    KEY_UP = 9
    TEXT_CHANGED = 10
    VALUE_CHANGED = 11
    STATE_CHANGED = 12

# ============================================================================
# Tool Class
# ============================================================================

class Tool:
    """Base tool/widget class"""
    
    def __init__(self, tool_type: ToolType, tool_id: str = None):
        self._id = tool_id or f"tool_{id(self)}"
        self._type = tool_type
        self._state = ToolState.NORMAL
        self._label = ""
        self._tooltip = ""
        self._icon = ""
        self._value = ""
        self._enabled = True
        self._visible = True
        self._x = 0
        self._y = 0
        self._width = 32
        self._height = 32
        self._parent = None
        self._children: List[Tool] = []
        self._event_handlers: Dict[ToolEvent, List[Callable]] = {}
        self._custom_properties: Dict[str, str] = {}
        self._user_data: Any = None
        
    # ========================================================================
    # Properties
    # ========================================================================
    
    @property
    def id(self) -> str:
        return self._id
    
    @id.setter
    def id(self, value: str):
        self._id = value
    
    @property
    def type(self) -> ToolType:
        return self._type
    
    @property
    def state(self) -> ToolState:
        return self._state
    
    @state.setter
    def state(self, value: ToolState):
        if self._state != value:
            self._state = value
            self._dispatch_event(ToolEvent.STATE_CHANGED)
    
    @property
    def label(self) -> str:
        return self._label
    
    @label.setter
    def label(self, value: str):
        self._label = value
    
    @property
    def tooltip(self) -> str:
        return self._tooltip
    
    @tooltip.setter
    def tooltip(self, value: str):
        self._tooltip = value
    
    @property
    def icon(self) -> str:
        return self._icon
    
    @icon.setter
    def icon(self, value: str):
        self._icon = value
    
    @property
    def value(self) -> str:
        return self._value
    
    @value.setter
    def value(self, value: str):
        if self._value != value:
            self._value = value
            self._dispatch_event(ToolEvent.VALUE_CHANGED)
    
    @property
    def enabled(self) -> bool:
        return self._enabled
    
    @enabled.setter
    def enabled(self, value: bool):
        self._enabled = value
    
    @property
    def visible(self) -> bool:
        return self._visible
    
    @visible.setter
    def visible(self, value: bool):
        self._visible = value
    
    @property
    def x(self) -> int:
        return self._x
    
    @x.setter
    def x(self, value: int):
        self._x = value
    
    @property
    def y(self) -> int:
        return self._y
    
    @y.setter
    def y(self, value: int):
        self._y = value
    
    @property
    def width(self) -> int:
        return self._width
    
    @width.setter
    def width(self, value: int):
        self._width = max(1, value)
    
    @property
    def height(self) -> int:
        return self._height
    
    @height.setter
    def height(self, value: int):
        self._height = max(1, value)
    
    @property
    def parent(self) -> Optional['Tool']:
        return self._parent
    
    @property
    def children(self) -> List['Tool']:
        return self._children.copy()
    
    @property
    def user_data(self) -> Any:
        return self._user_data
    
    @user_data.setter
    def user_data(self, value: Any):
        self._user_data = value
    
    # ========================================================================
    # Methods
    # ========================================================================
    
    def add_child(self, child: 'Tool'):
        """Add a child tool"""
        if child not in self._children:
            child._parent = self
            self._children.append(child)
    
    def remove_child(self, child: 'Tool'):
        """Remove a child tool"""
        if child in self._children:
            self._children.remove(child)
            child._parent = None
    
    def get_child_by_id(self, tool_id: str) -> Optional['Tool']:
        """Get child by ID"""
        for child in self._children:
            if child.id == tool_id:
                return child
            result = child.get_child_by_id(tool_id)
            if result:
                return result
        return None
    
    def get_children_by_type(self, tool_type: ToolType) -> List['Tool']:
        """Get children by type"""
        result = []
        for child in self._children:
            if child.type == tool_type:
                result.append(child)
        return result
    
    def add_event_listener(self, event: ToolEvent, callback: Callable):
        """Add an event listener"""
        if event not in self._event_handlers:
            self._event_handlers[event] = []
        self._event_handlers[event].append(callback)
    
    def remove_event_listener(self, event: ToolEvent, callback: Callable = None):
        """Remove an event listener"""
        if event in self._event_handlers:
            if callback:
                self._event_handlers[event].remove(callback)
            else:
                self._event_handlers[event].clear()
    
    def _dispatch_event(self, event: ToolEvent, data: Any = None):
        """Dispatch an event"""
        if event in self._event_handlers:
            for callback in self._event_handlers[event]:
                callback(data)
    
    def click(self):
        """Simulate a click"""
        self._dispatch_event(ToolEvent.CLICK)
    
    def set_custom_property(self, key: str, value: str):
        """Set a custom property"""
        self._custom_properties[key] = value
    
    def get_custom_property(self, key: str, default: str = "") -> str:
        """Get a custom property"""
        return self._custom_properties.get(key, default)
    
    def get_bounds(self) -> tuple:
        """Get the tool bounds"""
        return (self._x, self._y, self._width, self._height)
    
    def contains_point(self, px: int, py: int) -> bool:
        """Check if a point is inside the tool"""
        return (self._x <= px <= self._x + self._width and
                self._y <= py <= self._y + self._height)
    
    def render(self, context):
        """Render the tool"""
        # Override in subclasses
        pass
    
    def update(self):
        """Update the tool state"""
        # Override in subclasses
        pass
    
    def to_dict(self) -> Dict:
        """Convert to dictionary"""
        return {
            'id': self._id,
            'type': self._type.value,
            'label': self._label,
            'tooltip': self._tooltip,
            'icon': self._icon,
            'value': self._value,
            'enabled': self._enabled,
            'visible': self._visible,
            'x': self._x,
            'y': self._y,
            'width': self._width,
            'height': self._height,
            'custom_properties': self._custom_properties
        }
    
    def from_dict(self, data: Dict):
        """Load from dictionary"""
        self._id = data.get('id', self._id)
        self._label = data.get('label', self._label)
        self._tooltip = data.get('tooltip', self._tooltip)
        self._icon = data.get('icon', self._icon)
        self._value = data.get('value', self._value)
        self._enabled = data.get('enabled', self._enabled)
        self._visible = data.get('visible', self._visible)
        self._x = data.get('x', self._x)
        self._y = data.get('y', self._y)
        self._width = data.get('width', self._width)
        self._height = data.get('height', self._height)
        self._custom_properties = data.get('custom_properties', {})
    
    def __repr__(self) -> str:
        return f"<Tool id='{self.id}' label='{self.label}' type={self.type.name}>"

# ============================================================================
# Specific Tool Classes
# ============================================================================

class Button(Tool):
    """Button tool"""
    
    def __init__(self, label: str = "", icon: str = ""):
        super().__init__(ToolType.BUTTON)
        self.label = label
        self.icon = icon
        self._width = 80
        self._height = 32
    
    def render(self, context):
        """Render a button"""
        # Platform-specific rendering would go here
        pass

class Toggle(Tool):
    """Toggle/Checkbox tool"""
    
    def __init__(self, label: str = "", checked: bool = False):
        super().__init__(ToolType.TOGGLE)
        self.label = label
        self._checked = checked
        self._width = 80
        self._height = 32
    
    @property
    def checked(self) -> bool:
        return self._checked
    
    @checked.setter
    def checked(self, value: bool):
        if self._checked != value:
            self._checked = value
            self.state = ToolState.CHECKED if value else ToolState.UNCHECKED
            self._dispatch_event(ToolEvent.VALUE_CHANGED)

class Dropdown(Tool):
    """Dropdown menu tool"""
    
    def __init__(self, items: List[str] = None):
        super().__init__(ToolType.DROPDOWN)
        self._items = items or []
        self._selected_index = -1
        self._width = 120
        self._height = 32
    
    @property
    def items(self) -> List[str]:
        return self._items.copy()
    
    @items.setter
    def items(self, value: List[str]):
        self._items = value
        if self._selected_index >= len(self._items):
            self._selected_index = -1
    
    @property
    def selected_index(self) -> int:
        return self._selected_index
    
    @selected_index.setter
    def selected_index(self, value: int):
        if 0 <= value < len(self._items):
            self._selected_index = value
            self.value = self._items[value]
            self._dispatch_event(ToolEvent.VALUE_CHANGED)
    
    @property
    def selected_text(self) -> str:
        if 0 <= self._selected_index < len(self._items):
            return self._items[self._selected_index]
        return ""

class Slider(Tool):
    """Slider tool"""
    
    def __init__(self, min_value: int = 0, max_value: int = 100, value: int = 50):
        super().__init__(ToolType.SLIDER)
        self._min = min_value
        self._max = max_value
        self.value = str(value)
        self._width = 120
        self._height = 20
    
    @property
    def min_value(self) -> int:
        return self._min
    
    @property
    def max_value(self) -> int:
        return self._max
    
    @property
    def int_value(self) -> int:
        try:
            return int(self._value)
        except ValueError:
            return self._min
    
    @int_value.setter
    def int_value(self, value: int):
        self.value = str(max(self._min, min(self._max, value)))

class TextInput(Tool):
    """Text input tool"""
    
    def __init__(self, placeholder: str = ""):
        super().__init__(ToolType.TEXT_INPUT)
        self._placeholder = placeholder
        self._width = 150
        self._height = 24
    
    @property
    def placeholder(self) -> str:
        return self._placeholder
    
    @placeholder.setter
    def placeholder(self, value: str):
        self._placeholder = value

class AddressBar(Tool):
    """Address bar tool"""
    
    def __init__(self):
        super().__init__(ToolType.ADDRESS_BAR)
        self._width = 400
        self._height = 28
        self._placeholder = "Enter URL..."
        self._is_secure = False
        self._security_icon = "🔒"
    
    @property
    def is_secure(self) -> bool:
        return self._is_secure
    
    @is_secure.setter
    def is_secure(self, value: bool):
        self._is_secure = value
        self._security_icon = "🔒" if value else "🔓"

class Menu(Tool):
    """Menu tool"""
    
    def __init__(self, items: List[str] = None):
        super().__init__(ToolType.MENU)
        self._items = items or []
        self._width = 100
        self._height = 32
    
    @property
    def items(self) -> List[str]:
        return self._items.copy()
    
    @items.setter
    def items(self, value: List[str]):
        self._items = value

# ============================================================================
# Tool Factory
# ============================================================================

class ToolFactory:
    """Factory for creating tools"""
    
    @staticmethod
    def create_button(label: str = "", icon: str = "") -> Button:
        return Button(label, icon)
    
    @staticmethod
    def create_toggle(label: str = "", checked: bool = False) -> Toggle:
        return Toggle(label, checked)
    
    @staticmethod
    def create_dropdown(items: List[str] = None) -> Dropdown:
        return Dropdown(items)
    
    @staticmethod
    def create_slider(min_val: int = 0, max_val: int = 100, value: int = 50) -> Slider:
        return Slider(min_val, max_val, value)
    
    @staticmethod
    def create_text_input(placeholder: str = "") -> TextInput:
        return TextInput(placeholder)
    
    @staticmethod
    def create_address_bar() -> AddressBar:
        return AddressBar()
    
    @staticmethod
    def create_menu(items: List[str] = None) -> Menu:
        return Menu(items)
    
    @staticmethod
    def create_separator() -> Tool:
        tool = Tool(ToolType.SEPARATOR)
        tool.width = 2
        return tool