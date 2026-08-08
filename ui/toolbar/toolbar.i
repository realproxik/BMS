// toolbar.i - SWIG Interface for Toolbar
%module bms_toolbar

%{
#include "toolbar.h"
#include "tool.h"
#include <string>
#include <vector>
#include <memory>
%}

%include <std_string.i>
%include <std_vector.i>
%include <std_shared_ptr.i>
%include <std_unique_ptr.i>

%ignore BMS::UI::IToolbar::render;
%ignore BMS::UI::IToolbar::onToolAdded;
%ignore BMS::UI::IToolbar::onToolRemoved;
%ignore BMS::UI::IToolbar::onToolClicked;
%ignore BMS::UI::IToolbar::onToolbarShown;
%ignore BMS::UI::IToolbar::onToolbarHidden;

%include "toolbar.h"

%template(ToolbarPtr) std::unique_ptr<BMS::UI::IToolbar>;

%extend BMS::UI::IToolbar {
    %pythoncode {
        def __repr__(self):
            return f"<Toolbar tools={self.getToolCount()}>"
        
        def add_button(self, label, icon="", callback=None):
            """Add a button to the toolbar"""
            from . import tool
            button = tool.Button(label, icon)
            if callback:
                button.add_event_listener(tool.ToolEvent.CLICK, callback)
            self.addTool(button)
            return button
        
        def add_separator(self):
            """Add a separator"""
            from . import tool
            sep = tool.Tool(tool.ToolType.SEPARATOR)
            self.addTool(sep)
            return sep
        
        def add_dropdown(self, items, callback=None):
            """Add a dropdown menu"""
            from . import tool
            dropdown = tool.Dropdown(items)
            if callback:
                dropdown.add_event_listener(tool.ToolEvent.VALUE_CHANGED, callback)
            self.addTool(dropdown)
            return dropdown
        
        def add_text_input(self, placeholder="", callback=None):
            """Add a text input"""
            from . import tool
            input = tool.TextInput(placeholder)
            if callback:
                input.add_event_listener(tool.ToolEvent.TEXT_CHANGED, callback)
            self.addTool(input)
            return input
        
        def add_address_bar(self, callback=None):
            """Add an address bar"""
            from . import tool
            bar = tool.AddressBar()
            if callback:
                bar.add_event_listener(tool.ToolEvent.TEXT_CHANGED, callback)
            self.addTool(bar)
            return bar
        
        def __getitem__(self, index):
            """Get tool by index"""
            return self.getTool(index)
        
        def __len__(self):
            """Get number of tools"""
            return self.getToolCount()
        
        def __iter__(self):
            """Iterate over tools"""
            for i in range(self.getToolCount()):
                yield self.getTool(i)
        
        @property
        def tools(self):
            """Get all tools as a list"""
            return list(self)
        
        @property
        def orientation(self):
            return self.getOrientation()
        
        @orientation.setter
        def orientation(self, value):
            self.setOrientation(value)
        
        @property
        def spacing(self):
            return self.getSpacing()
        
        @spacing.setter
        def spacing(self, value):
            self.setSpacing(value)
        
        @property
        def padding(self):
            return self.getPadding()
        
        @padding.setter
        def padding(self, value):
            self.setPadding(value)
        
        @property
        def visible(self):
            return self.isVisible()
        
        @visible.setter
        def visible(self, value):
            if value:
                self.show()
            else:
                self.hide()
    }
}