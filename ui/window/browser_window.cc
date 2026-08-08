// ui/browser_window.cpp
#include "browser_window.h"
#include <sstream>
#include <iostream>

namespace BMS {

BrowserWindow::BrowserWindow(int width, int height, const std::string& title)
    : width_(width), height_(height), title_(title) {
    layoutEngine_ = std::make_unique<LayoutEngine>(width, height);
    paintEngine_ = std::make_unique<PaintEngine>(width, height);
    initWindow();
}

BrowserWindow::~BrowserWindow() {
    close();
}

void BrowserWindow::initWindow() {
#ifdef _WIN32
    // Windows implementation
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        BrowserWindow* window = reinterpret_cast<BrowserWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (window) {
            switch (msg) {
                case WM_PAINT:
                    window->renderFrame();
                    break;
                case WM_DESTROY:
                    PostQuitMessage(0);
                    break;
                default:
                    return DefWindowProc(hwnd, msg, wParam, lParam);
            }
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    };
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "BMSBrowser";
    
    RegisterClassEx(&wc);
    
    hwnd_ = CreateWindowEx(
        0, "BMSBrowser", title_.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width_, height_,
        nullptr, nullptr, wc.hInstance, nullptr
    );
    
    SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    
    hdc_ = GetDC(hwnd_);
#elif __APPLE__
    // MacOS implementation would go here
    std::cout << "MacOS window created" << std::endl;
#elif __linux__
    // Linux implementation would go here
    std::cout << "Linux window created" << std::endl;
#endif
}

void BrowserWindow::show() {
#ifdef _WIN32
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif
}

void BrowserWindow::renderFrame() {
#ifdef _WIN32
    // Clear background
    RECT rect;
    GetClientRect(hwnd_, &rect);
    HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc_, &rect, brush);
    DeleteObject(brush);
    
    // Draw simple text
    if (document_) {
        // Use layout engine to compute layout
        LayoutBox layout = layoutEngine_->computeLayout(document_.get());
        
        // Paint the layout
        paintEngine_->paint(hdc_, layout);
    }
#endif
}

void BrowserWindow::render(const std::string& html) {
    // Parse HTML into document
    HTMLParser parser;
    document_ = parser.parse(html);
    
    // Update title if available
    if (document_) {
        std::string title = document_->getTitle();
        if (!title.empty()) {
#ifdef _WIN32
            SetWindowText(hwnd_, ("BMS Browser - " + title).c_str());
#endif
        }
    }
    
    // Trigger render
    renderFrame();
}

void BrowserWindow::navigate(const std::string& url) {
    currentURL_ = url;
    
    // Create HTTP request
    HTTPRequest request;
    request.setMethod(HTTPRequest::Method::GET);
    request.setURL(url);
    request.setHeader("User-Agent", "BMS-Browser/1.0");
    
    // Send request asynchronously
    request.sendAsync([this](const HTTPRequest::Response& response) {
        if (response.statusCode == 200) {
            render(response.body);
        } else {
            std::string errorHTML = "<html><body><h1>Error</h1><p>Failed to load: " + 
                                     std::to_string(response.statusCode) + "</p></body></html>";
            render(errorHTML);
        }
    });
}

void BrowserWindow::refresh() {
    if (!currentURL_.empty()) {
        navigate(currentURL_);
    }
}

void BrowserWindow::close() {
#ifdef _WIN32
    if (hdc_) {
        ReleaseDC(hwnd_, hdc_);
        hdc_ = nullptr;
    }
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
#endif
}

} // namespace BMS