// api.i - SWIG Interface for Python bindings
%module bms_api

%{
#include "api.h"
#include "api.hh"
#include <string>
#include <vector>
#include <memory>
%}

// Handle std::shared_ptr
%include <std_shared_ptr.i>
%include <std_string.i>
%include <std_vector.i>
%include <std_unordered_map.i>

// Define vectors and maps
%template(StringVector) std::vector<std::string>;
%template(StringMap) std::unordered_map<std::string, std::string>;

// Ignore some methods
%ignore BMS::API::IBrowser::onPageLoad;
%ignore BMS::API::IBrowser::onTabOpen;
%ignore BMS::API::IBrowser::onTabClose;
%ignore BMS::API::IBrowser::onError;

// Include header files
%include "api.h"
%include "api.hh"

// Python-specific extensions
%extend BMS::API::IBrowser {
    %pythoncode {
        def on_page_load(self, callback):
            """Register a page load callback"""
            self.onPageLoad(callback)
        
        def on_error(self, callback):
            """Register an error callback"""
            self.onError(callback)
        
        def __enter__(self):
            """Context manager entry"""
            return self
        
        def __exit__(self, exc_type, exc_val, exc_tb):
            """Context manager exit"""
            self.shutdown()
    }
}

%extend BMS::API::Window {
    %pythoncode {
        def __repr__(self):
            return f"<Window title='{self.getTitle()}' tabs={len(self.getTabs())}>"
    }
}

%extend BMS::API::Tab {
    %pythoncode {
        def __repr__(self):
            return f"<Tab url='{self.getURL()}' title='{self.getTitle()}'>"
    }
}

%extend BMS::API::Document {
    %pythoncode {
        def __repr__(self):
            return f"<Document title='{self.getTitle()}' url='{self.getURL()}'>"
    }
}

%extend BMS::API::Element {
    %pythoncode {
        def __repr__(self):
            return f"<Element tag='{self.getTagName()}' id='{self.getId()}'>"
        
        def __getitem__(self, key):
            """Get attribute by key"""
            return self.getAttribute(key)
        
        def __setitem__(self, key, value):
            """Set attribute by key"""
            self.setAttribute(key, value)
        
        @property
        def innerHTML(self):
            return self.getInnerHTML()
        
        @innerHTML.setter
        def innerHTML(self, value):
            self.setInnerHTML(value)
    }
}