// tool.i - SWIG Interface for Tool
%module bms_tool

%{
#include "tool.h"
#include "toolbar.h"
#include <string>
#include <vector>
#include <memory>
%}

// Handle STL types
%include <std_string.i>
%include <std_vector.i>
%include <std_shared_ptr.i>
%include <std_function.i>

// Define vector types
%template(ToolVector) std::vector<BMS::UI::ITool*>;
%template(StringVector) std::vector<std::string>;

// Ignore some methods that use function pointers
%ignore BMS::UI::ITool::addEventListener;
%ignore BMS::UI::ITool::removeEventListener;
%ignore BMS::UI::ITool::render;

// Include headers
%include "tool.h"
%include "toolbar.h"

// Python extensions
%extend BMS::UI::ITool {
    %pythoncode {
        def __repr__(self):
            return f"<Tool id='{self.getId()}' label='{self.getLabel()}' type={self.getType()}>"
        
        def on_click(self, callback):
            """Register a click event handler"""
            self.addEventListener(BMS::UI::ToolEvent::CLICK, 
                lambda e: callback())
        
        def on_value_changed(self, callback):
            """Register a value changed event handler"""
            self.addEventListener(BMS::UI::ToolEvent::VALUE_CHANGED,
                lambda e: callback(e.value))
        
        @property
        def label(self):
            return self.getLabel()
        
        @label.setter
        def label(self, value):
            self.setLabel(value)
        
        @property
        def value(self):
            return self.getValue()
        
        @value.setter
        def value(self, v):
            self.setValue(v)
        
        @property
        def enabled(self):
            return self.isEnabled()
        
        @enabled.setter
        def enabled(self, value):
            self.setEnabled(value)
        
        @property
        def visible(self):
            return self.isVisible()
        
        @visible.setter
        def visible(self, value):
            self.setVisible(value)
    }
}

%extend BMS::UI::Toolbar {
    %pythoncode {
        def __repr__(self):
            return f"<Toolbar tools={self.getToolCount()}>"
        
        def add_button(self, label, icon="", callback=None):
            """Add a button to the toolbar"""
            button = BMS::UI::ToolFactory.createButton(label)
            if icon:
                button.setIcon(icon)
            if callback:
                button.addEventListener(BMS::UI::ToolEvent::CLICK, 
                    lambda e: callback())
            self.addTool(button.release())
            return button
        
        def add_separator(self):
            """Add a separator to the toolbar"""
            sep = BMS::UI::ToolFactory.createSeparator()
            self.addTool(sep.release())
            return sep
        
        def __getitem__(self, index):
            """Get tool by index"""
            return self.getTool(index)
        
        def __len__(self):
            """Get number of tools"""
            return self.getToolCount()
    }
}