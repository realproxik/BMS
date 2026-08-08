// engine/paint/paint_engine.cpp
#include "paint_engine.h"
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

namespace BMS {

PaintEngine::PaintEngine(int width, int height)
    : width_(width), height_(height) {
}

void PaintEngine::paint(void* deviceContext, const LayoutBox& layoutBox) {
#ifdef _WIN32
    HDC hdc = static_cast<HDC>(deviceContext);
    if (!hdc) return;
    
    // Save current DC state
    int savedDC = SaveDC(hdc);
    
    // Paint this node
    paintNode(hdc, layoutBox);
    
    // Paint children recursively
    for (const auto& child : layoutBox.children) {
        paint(hdc, child);
    }
    
    // Restore DC state
    RestoreDC(hdc, savedDC);
#endif
}

void PaintEngine::paintNode(void* deviceContext, const LayoutBox& layoutBox) {
#ifdef _WIN32
    HDC hdc = static_cast<HDC>(deviceContext);
    if (!hdc || !layoutBox.node) return;
    
    // Get element styles
    std::unordered_map<std::string, std::string> styles;
    if (layoutBox.node->getType() == Node::NodeType::ELEMENT) {
        Element* element = static_cast<Element*>(layoutBox.node);
        // In a real implementation, use StyleResolver
        // For now, just get inline styles
        styles = element->getAllStyles();
    }
    
    // Determine background color
    COLORREF bgColor = RGB(255, 255, 255);
    auto bgIt = styles.find("background-color");
    if (bgIt != styles.end()) {
        bgColor = parseColor(bgIt->second);
    }
    
    // Determine text color
    COLORREF textColor = RGB(0, 0, 0);
    auto colorIt = styles.find("color");
    if (colorIt != styles.end()) {
        textColor = parseColor(colorIt->second);
    }
    
    // Draw background
    HBRUSH bgBrush = CreateSolidBrush(bgColor);
    RECT rect = {
        layoutBox.x,
        layoutBox.y,
        layoutBox.x + layoutBox.width,
        layoutBox.y + layoutBox.height
    };
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    // Draw text if it's a text node
    if (layoutBox.node->getType() == Node::NodeType::TEXT) {
        std::string text = layoutBox.node->getNodeValue();
        if (!text.empty()) {
            SetTextColor(hdc, textColor);
            SetBkMode(hdc, TRANSPARENT);
            TextOut(hdc, layoutBox.x + 2, layoutBox.y + 2, text.c_str(), text.length());
        }
    }
    
    // Draw border if specified
    auto borderIt = styles.find("border");
    if (borderIt != styles.end()) {
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
        SelectObject(hdc, borderPen);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, layoutBox.x, layoutBox.y, 
                  layoutBox.x + layoutBox.width, 
                  layoutBox.y + layoutBox.height);
        DeleteObject(borderPen);
    }
#endif
}

COLORREF PaintEngine::parseColor(const std::string& color) {
    // Simple color parsing
    if (color == "red") return RGB(255, 0, 0);
    if (color == "green") return RGB(0, 255, 0);
    if (color == "blue") return RGB(0, 0, 255);
    if (color == "black") return RGB(0, 0, 0);
    if (color == "white") return RGB(255, 255, 255);
    if (color == "gray") return RGB(128, 128, 128);
    if (color == "yellow") return RGB(255, 255, 0);
    
    // Hex color: #RRGGBB
    if (color.length() == 7 && color[0] == '#') {
        int r = std::stoi(color.substr(1, 2), nullptr, 16);
        int g = std::stoi(color.substr(3, 2), nullptr, 16);
        int b = std::stoi(color.substr(5, 2), nullptr, 16);
        return RGB(r, g, b);
    }
    
    return RGB(0, 0, 0);
}

} // namespace BMS