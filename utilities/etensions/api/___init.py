# ___init.py - Python Package Initialization
"""
BMS Browser - Python Bindings
================================

A full-featured web browser built from scratch with Python bindings.

Usage:
    >>> import bms
    >>> browser = bms.create_browser()
    >>> window = browser.create_window("https://example.com")
    >>> window.show()
"""

__version__ = "1.1.0"
__author__ = "BMS Browser Team"
__all__ = [
    'Browser',
    'Window', 
    'Tab',
    'Document',
    'Element',
    'create_browser',
    'get_browser',
    'initialize',
    'shutdown'
]

import sys
import os
from typing import Optional, List, Dict, Any, Callable

# Import SWIG-generated modules
try:
    from . import _bms_core as core
    from . import _bms_network as network
    from . import _bms_ui as ui
    from . import _bms_api as api
except ImportError as e:
    print(f"Warning: Could not import C++ modules: {e}", file=sys.stderr)
    # Fallback to pure Python implementations
    from . import _bms_pure_python as api

# ============================================================================
# High-level Python wrappers
# ============================================================================

class Browser:
    """
    High-level browser wrapper for Python
    """
    
    def __init__(self):
        self._impl = api.createBrowser()
        self._windows: List[Window] = []
        self._tabs: List[Tab] = []
        self._callbacks: Dict[str, List[Callable]] = {
            'page_load': [],
            'tab_open': [],
            'tab_close': [],
            'error': []
        }
    
    def initialize(self, args: Optional[List[str]] = None) -> bool:
        """Initialize the browser"""
        if args is None:
            args = ['bms']
        return self._impl.initialize(len(args), args)
    
    def shutdown(self):
        """Shutdown the browser"""
        self._impl.shutdown()
        for callback in self._callbacks.get('shutdown', []):
            callback()
    
    def create_window(self, url: str = "", width: int = 1024, height: int = 768) -> 'Window':
        """Create a new browser window"""
        window_impl = self._impl.createWindow(url, width, height)
        window = Window(window_impl, self)
        self._windows.append(window)
        
        for callback in self._callbacks.get('window_open', []):
            callback(window)
        
        return window
    
    def close_window(self, window: 'Window'):
        """Close a browser window"""
        if window in self._windows:
            self._windows.remove(window)
            self._impl.closeWindow(window._impl)
            for callback in self._callbacks.get('window_close', []):
                callback(window)
    
    def create_tab(self, url: str = "", window: Optional['Window'] = None) -> 'Tab':
        """Create a new tab"""
        window_impl = window._impl if window else None
        tab_impl = self._impl.createTab(url, window_impl)
        tab = Tab(tab_impl, self, window)
        self._tabs.append(tab)
        
        if window:
            window._tabs.append(tab)
        
        for callback in self._callbacks.get('tab_open', []):
            callback(tab)
        
        return tab
    
    def close_tab(self, tab: 'Tab'):
        """Close a tab"""
        if tab in self._tabs:
            self._tabs.remove(tab)
            self._impl.closeTab(tab._impl)
            
            if tab._window:
                tab._window._tabs.remove(tab)
            
            for callback in self._callbacks.get('tab_close', []):
                callback(tab)
    
    def navigate(self, url: str, tab: Optional['Tab'] = None):
        """Navigate to a URL"""
        tab_impl = tab._impl if tab else None
        self._impl.navigate(url, tab_impl)
        
        for callback in self._callbacks.get('navigate', []):
            callback(url, tab)
    
    def get_windows(self) -> List['Window']:
        """Get all windows"""
        return self._windows.copy()
    
    def get_tabs(self) -> List['Tab']:
        """Get all tabs"""
        return self._tabs.copy()
    
    def get_active_window(self) -> Optional['Window']:
        """Get the active window"""
        impl = self._impl.getActiveWindow()
        for window in self._windows:
            if window._impl == impl:
                return window
        return None
    
    def get_active_tab(self) -> Optional['Tab']:
        """Get the active tab"""
        impl = self._impl.getActiveTab()
        for tab in self._tabs:
            if tab._impl == impl:
                return tab
        return None
    
    def on(self, event: str, callback: Callable):
        """Register an event callback"""
        if event not in self._callbacks:
            self._callbacks[event] = []
        self._callbacks[event].append(callback)
    
    def off(self, event: str, callback: Callable):
        """Unregister an event callback"""
        if event in self._callbacks and callback in self._callbacks[event]:
            self._callbacks[event].remove(callback)
    
    def set_setting(self, key: str, value: str):
        """Set a browser setting"""
        self._impl.setSetting(key, value)
    
    def get_setting(self, key: str) -> str:
        """Get a browser setting"""
        return self._impl.getSetting(key)
    
    def save_settings(self):
        """Save browser settings"""
        self._impl.saveSettings()
    
    def load_settings(self):
        """Load browser settings"""
        self._impl.loadSettings()
    
    # v1.1 features
    def set_user_agent(self, user_agent: str):
        """Set the user agent string"""
        if hasattr(self._impl, 'setUserAgent'):
            self._impl.setUserAgent(user_agent)
    
    def get_user_agent(self) -> str:
        """Get the user agent string"""
        if hasattr(self._impl, 'getUserAgent'):
            return self._impl.getUserAgent()
        return "BMS-Browser/1.1"
    
    def enable_incognito(self, enabled: bool = True):
        """Enable or disable incognito mode"""
        if hasattr(self._impl, 'enableIncognitoMode'):
            self._impl.enableIncognitoMode(enabled)
    
    def is_incognito(self) -> bool:
        """Check if incognito mode is enabled"""
        if hasattr(self._impl, 'isIncognitoMode'):
            return self._impl.isIncognitoMode()
        return False
    
    def clear_cache(self):
        """Clear the browser cache"""
        if hasattr(self._impl, 'clearCache'):
            self._impl.clearCache()
    
    def clear_cookies(self):
        """Clear all cookies"""
        if hasattr(self._impl, 'clearCookies'):
            self._impl.clearCookies()
    
    def clear_all_data(self):
        """Clear all browsing data"""
        if hasattr(self._impl, 'clearAllData'):
            self._impl.clearAllData()
    
    def __enter__(self):
        """Context manager entry"""
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.shutdown()
    
    def __repr__(self) -> str:
        return f"<Browser windows={len(self._windows)} tabs={len(self._tabs)}>"


class Window:
    """Browser window wrapper"""
    
    def __init__(self, impl, browser: Browser):
        self._impl = impl
        self._browser = browser
        self._tabs: List[Tab] = []
    
    def show(self):
        """Show the window"""
        self._impl.show()
    
    def hide(self):
        """Hide the window"""
        self._impl.hide()
    
    def maximize(self):
        """Maximize the window"""
        self._impl.maximize()
    
    def minimize(self):
        """Minimize the window"""
        self._impl.minimize()
    
    def restore(self):
        """Restore the window"""
        self._impl.restore()
    
    def is_visible(self) -> bool:
        """Check if the window is visible"""
        return self._impl.isVisible()
    
    def is_maximized(self) -> bool:
        """Check if the window is maximized"""
        return self._impl.isMaximized()
    
    def is_minimized(self) -> bool:
        """Check if the window is minimized"""
        return self._impl.isMinimized()
    
    def set_title(self, title: str):
        """Set the window title"""
        self._impl.setTitle(title)
    
    def get_title(self) -> str:
        """Get the window title"""
        return self._impl.getTitle()
    
    def set_size(self, width: int, height: int):
        """Set the window size"""
        self._impl.setSize(width, height)
    
    def get_size(self) -> tuple:
        """Get the window size"""
        width, height = 0, 0
        self._impl.getSize(width, height)
        return (width, height)
    
    def set_position(self, x: int, y: int):
        """Set the window position"""
        self._impl.setPosition(x, y)
    
    def get_position(self) -> tuple:
        """Get the window position"""
        x, y = 0, 0
        self._impl.getPosition(x, y)
        return (x, y)
    
    def set_fullscreen(self, fullscreen: bool):
        """Set fullscreen mode"""
        self._impl.setFullscreen(fullscreen)
    
    def is_fullscreen(self) -> bool:
        """Check if the window is in fullscreen mode"""
        return self._impl.isFullscreen()
    
    def add_tab(self, tab: 'Tab'):
        """Add a tab to this window"""
        self._tabs.append(tab)
        self._impl.addTab(tab._impl)
        tab._window = self
    
    def remove_tab(self, tab: 'Tab'):
        """Remove a tab from this window"""
        if tab in self._tabs:
            self._tabs.remove(tab)
            self._impl.removeTab(tab._impl)
            tab._window = None
    
    def get_tabs(self) -> List['Tab']:
        """Get all tabs in this window"""
        return self._tabs.copy()
    
    def get_active_tab(self) -> Optional['Tab']:
        """Get the active tab in this window"""
        impl = self._impl.getActiveTab()
        for tab in self._tabs:
            if tab._impl == impl:
                return tab
        return None
    
    def set_active_tab(self, tab: 'Tab'):
        """Set the active tab"""
        if tab in self._tabs:
            self._impl.setActiveTab(tab._impl)
    
    def get_native_handle(self):
        """Get the native window handle"""
        return self._impl.getNativeHandle()
    
    def __repr__(self) -> str:
        return f"<Window title='{self.get_title()}' tabs={len(self._tabs)}>"


class Tab:
    """Browser tab wrapper"""
    
    def __init__(self, impl, browser: Browser, window: Optional[Window] = None):
        self._impl = impl
        self._browser = browser
        self._window = window
        self._callbacks: Dict[str, List[Callable]] = {
            'load_start': [],
            'load_finish': [],
            'load_error': []
        }
    
    def load_url(self, url: str):
        """Load a URL in this tab"""
        self._impl.loadURL(url)
    
    def get_url(self) -> str:
        """Get the current URL"""
        return self._impl.getURL()
    
    def get_title(self) -> str:
        """Get the tab title"""
        return self._impl.getTitle()
    
    def reload(self):
        """Reload the current page"""
        self._impl.reload()
    
    def stop(self):
        """Stop loading"""
        self._impl.stop()
    
    def go_back(self):
        """Go back in history"""
        self._impl.goBack()
    
    def go_forward(self):
        """Go forward in history"""
        self._impl.goForward()
    
    def can_go_back(self) -> bool:
        """Check if can go back"""
        return self._impl.canGoBack()
    
    def can_go_forward(self) -> bool:
        """Check if can go forward"""
        return self._impl.canGoForward()
    
    def get_document(self) -> 'Document':
        """Get the document object"""
        return Document(self._impl.getDocument(), self)
    
    def execute_script(self, script: str):
        """Execute JavaScript in the tab"""
        self._impl.executeScript(script)
    
    def evaluate_script(self, script: str) -> str:
        """Evaluate JavaScript and return the result"""
        return self._impl.evaluateScript(script)
    
    def set_zoom(self, level: float):
        """Set the zoom level"""
        self._impl.setZoomLevel(level)
    
    def get_zoom(self) -> float:
        """Get the zoom level"""
        return self._impl.getZoomLevel()
    
    def set_muted(self, muted: bool):
        """Mute or unmute the tab"""
        self._impl.setMuted(muted)
    
    def is_muted(self) -> bool:
        """Check if the tab is muted"""
        return self._impl.isMuted()
    
    def is_loading(self) -> bool:
        """Check if the tab is loading"""
        return self._impl.isLoading()
    
    def get_load_progress(self) -> float:
        """Get the load progress (0.0 to 1.0)"""
        return self._impl.getLoadProgress()
    
    def on(self, event: str, callback: Callable):
        """Register an event callback"""
        if event not in self._callbacks:
            self._callbacks[event] = []
        self._callbacks[event].append(callback)
        
        # Register with native implementation
        if event == 'load_start':
            self._impl.onLoadStart(callback)
        elif event == 'load_finish':
            self._impl.onLoadFinish(callback)
        elif event == 'load_error':
            self._impl.onLoadError(callback)
    
    def off(self, event: str, callback: Callable):
        """Unregister an event callback"""
        if event in self._callbacks and callback in self._callbacks[event]:
            self._callbacks[event].remove(callback)
    
    def get_window(self) -> Optional[Window]:
        """Get the parent window"""
        return self._window
    
    def close(self):
        """Close this tab"""
        self._browser.close_tab(self)
    
    def __repr__(self) -> str:
        return f"<Tab url='{self.get_url()}' title='{self.get_title()}'>"


class Document:
    """Document wrapper"""
    
    def __init__(self, impl, tab: Tab):
        self._impl = impl
        self._tab = tab
    
    def get_title(self) -> str:
        """Get the document title"""
        return self._impl.getTitle()
    
    def get_url(self) -> str:
        """Get the document URL"""
        return self._impl.getURL()
    
    def get_document_uri(self) -> str:
        """Get the document URI"""
        return self._impl.getDocumentURI()
    
    def get_element_by_id(self, id: str) -> Optional['Element']:
        """Get an element by ID"""
        impl = self._impl.getElementById(id)
        return Element(impl, self) if impl else None
    
    def get_elements_by_tag_name(self, tag: str) -> List['Element']:
        """Get elements by tag name"""
        impls = self._impl.getElementsByTagName(tag)
        return [Element(impl, self) for impl in impls]
    
    def get_elements_by_class_name(self, class_name: str) -> List['Element']:
        """Get elements by class name"""
        impls = self._impl.getElementsByClassName(class_name)
        return [Element(impl, self) for impl in impls]
    
    def query_selector(self, selector: str) -> Optional['Element']:
        """Query a single element"""
        impl = self._impl.querySelector(selector)
        return Element(impl, self) if impl else None
    
    def query_selector_all(self, selector: str) -> List['Element']:
        """Query multiple elements"""
        impls = self._impl.querySelectorAll(selector)
        return [Element(impl, self) for impl in impls]
    
    def create_element(self, tag_name: str) -> 'Element':
        """Create a new element"""
        impl = self._impl.createElement(tag_name)
        return Element(impl, self)
    
    def create_text_node(self, text: str) -> 'Element':
        """Create a text node"""
        impl = self._impl.createTextNode(text)
        return Element(impl, self)
    
    def get_html(self) -> str:
        """Get the document HTML"""
        return self._impl.getHTML()
    
    def set_html(self, html: str):
        """Set the document HTML"""
        self._impl.setHTML(html)
    
    def get_text_content(self) -> str:
        """Get the document text content"""
        return self._impl.getTextContent()
    
    def set_text_content(self, text: str):
        """Set the document text content"""
        self._impl.setTextContent(text)
    
    def is_ready(self) -> bool:
        """Check if the document is ready"""
        return self._impl.isReady()
    
    def wait_for_ready(self):
        """Wait for the document to be ready"""
        self._impl.waitForReady()
    
    def __repr__(self) -> str:
        return f"<Document title='{self.get_title()}' url='{self.get_url()}'>"


class Element:
    """DOM element wrapper"""
    
    def __init__(self, impl, document: Document):
        self._impl = impl
        self._document = document
    
    def get_tag_name(self) -> str:
        """Get the tag name"""
        return self._impl.getTagName()
    
    def get_id(self) -> str:
        """Get the element ID"""
        return self._impl.getId()
    
    def set_id(self, id: str):
        """Set the element ID"""
        self._impl.setId(id)
    
    def get_class_name(self) -> str:
        """Get the class name"""
        return self._impl.getClassName()
    
    def set_class_name(self, class_name: str):
        """Set the class name"""
        self._impl.setClassName(class_name)
    
    def get_attribute(self, name: str) -> str:
        """Get an attribute value"""
        return self._impl.getAttribute(name)
    
    def set_attribute(self, name: str, value: str):
        """Set an attribute value"""
        self._impl.setAttribute(name, value)
    
    def has_attribute(self, name: str) -> bool:
        """Check if an attribute exists"""
        return self._impl.hasAttribute(name)
    
    def remove_attribute(self, name: str):
        """Remove an attribute"""
        self._impl.removeAttribute(name)
    
    def get_style(self, property: str) -> str:
        """Get a style property value"""
        return self._impl.getStyle(property)
    
    def set_style(self, property: str, value: str):
        """Set a style property value"""
        self._impl.setStyle(property, value)
    
    def get_text_content(self) -> str:
        """Get the text content"""
        return self._impl.getTextContent()
    
    def set_text_content(self, text: str):
        """Set the text content"""
        self._impl.setTextContent(text)
    
    def get_inner_html(self) -> str:
        """Get the inner HTML"""
        return self._impl.getInnerHTML()
    
    def set_inner_html(self, html: str):
        """Set the inner HTML"""
        self._impl.setInnerHTML(html)
    
    def get_outer_html(self) -> str:
        """Get the outer HTML"""
        return self._impl.getOuterHTML()
    
    def get_parent(self) -> Optional['Element']:
        """Get the parent element"""
        impl = self._impl.getParent()
        return Element(impl, self._document) if impl else None
    
    def get_children(self) -> List['Element']:
        """Get child elements"""
        impls = self._impl.getChildren()
        return [Element(impl, self._document) for impl in impls]
    
    def get_first_child(self) -> Optional['Element']:
        """Get the first child"""
        impl = self._impl.getFirstChild()
        return Element(impl, self._document) if impl else None
    
    def get_last_child(self) -> Optional['Element']:
        """Get the last child"""
        impl = self._impl.getLastChild()
        return Element(impl, self._document) if impl else None
    
    def append_child(self, child: 'Element'):
        """Append a child element"""
        self._impl.appendChild(child._impl)
        child._document = self._document
    
    def remove_child(self, child: 'Element'):
        """Remove a child element"""
        self._impl.removeChild(child._impl)
        child._document = None
    
    def insert_before(self, new_child: 'Element', ref_child: 'Element'):
        """Insert a child before a reference element"""
        self._impl.insertBefore(new_child._impl, ref_child._impl)
        new_child._document = self._document
    
    def clone_node(self, deep: bool = True) -> 'Element':
        """Clone the element"""
        impl = self._impl.cloneNode(deep)
        return Element(impl, self._document)
    
    def add_event_listener(self, type: str, callback: Callable):
        """Add an event listener"""
        self._impl.addEventListener(type, callback)
    
    def remove_event_listener(self, type: str):
        """Remove an event listener"""
        self._impl.removeEventListener(type)
    
    def click(self):
        """Simulate a click"""
        self._impl.click()
    
    def focus(self):
        """Focus the element"""
        self._impl.focus()
    
    def blur(self):
        """Blur the element"""
        self._impl.blur()
    
    def scroll_into_view(self):
        """Scroll the element into view"""
        self._impl.scrollIntoView()
    
    def get_bounding_rect(self) -> tuple:
        """Get the bounding rectangle"""
        x, y, width, height = 0, 0, 0, 0
        self._impl.getBoundingRect(x, y, width, height)
        return (x, y, width, height)
    
    def __getitem__(self, key: str) -> str:
        """Get attribute by key"""
        return self.get_attribute(key)
    
    def __setitem__(self, key: str, value: str):
        """Set attribute by key"""
        self.set_attribute(key, value)
    
    def __repr__(self) -> str:
        return f"<Element tag='{self.get_tag_name()}' id='{self.get_id()}'>"


# ============================================================================
# Global Functions
# ============================================================================

_browser_instance: Optional[Browser] = None

def create_browser() -> Browser:
    """Create a new browser instance"""
    global _browser_instance
    _browser_instance = Browser()
    return _browser_instance

def get_browser() -> Optional[Browser]:
    """Get the global browser instance"""
    return _browser_instance

def initialize(args: Optional[List[str]] = None) -> bool:
    """Initialize the browser API"""
    browser = create_browser()
    return browser.initialize(args)

def shutdown():
    """Shutdown the browser API"""
    global _browser_instance
    if _browser_instance:
        _browser_instance.shutdown()
        _browser_instance = None

def get_version() -> str:
    """Get the API version"""
    return __version__

def get_author() -> str:
    """Get the author name"""
    return __author__

# ============================================================================
# Console entry point
# ============================================================================

if __name__ == "__main__":
    print("BMS Browser Python Module")
    print(f"Version: {__version__}")
    print(f"Author: {__author__}")
    print("\nUsage:")
    print("  import bms")
    print("  browser = bms.create_browser()")
    print("  browser.initialize()")
    print("  window = browser.create_window('https://example.com')")
    print("  window.show()")
    print("  browser.shutdown()")