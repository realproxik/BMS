// engine/core/style/style_resolver.cpp
#include "style_resolver.h"
#include <regex>
#include <sstream>
#include <algorithm>

namespace BMS {

// ==================== Style Resolver Implementation ====================

StyleResolver::StyleResolver() {
    // Initialize with default styles
    userAgentStylesheet_ = {
        {"html", {{"display", "block"}}},
        {"body", {{"display", "block"}, {"margin", "8px"}}},
        {"p", {{"display", "block"}, {"margin", "1em 0"}}},
        {"div", {{"display", "block"}}},
        {"span", {{"display", "inline"}}},
        {"h1", {{"display", "block"}, {"font-size", "2em"}, {"margin", "0.67em 0"}}},
        {"h2", {{"display", "block"}, {"font-size", "1.5em"}, {"margin", "0.83em 0"}}},
        {"h3", {{"display", "block"}, {"font-size", "1.17em"}, {"margin", "1em 0"}}},
        {"h4", {{"display", "block"}, {"font-size", "1em"}, {"margin", "1.33em 0"}}},
        {"h5", {{"display", "block"}, {"font-size", "0.83em"}, {"margin", "1.67em 0"}}},
        {"h6", {{"display", "block"}, {"font-size", "0.67em"}, {"margin", "2.33em 0"}}},
        {"ul", {{"display", "block"}, {"margin", "1em 0"}, {"padding-left", "40px"}}},
        {"ol", {{"display", "block"}, {"margin", "1em 0"}, {"padding-left", "40px"}}},
        {"li", {{"display", "list-item"}}},
        {"table", {{"display", "table"}, {"border-collapse", "collapse"}}},
        {"tr", {{"display", "table-row"}}},
        {"td", {{"display", "table-cell"}, {"border", "1px solid #ddd"}, {"padding", "8px"}}},
        {"th", {{"display", "table-cell"}, {"border", "1px solid #ddd"}, {"padding", "8px"}, {"font-weight", "bold"}}},
        {"a", {{"color", "#0066cc"}, {"text-decoration", "underline"}}},
        {"img", {{"display", "inline-block"}}},
        {"strong", {{"font-weight", "bold"}}},
        {"em", {{"font-style", "italic"}}},
        {"br", {{"display", "block"}}},
        {"hr", {{"display", "block"}, {"border", "1px solid #ccc"}, {"margin", "8px 0"}}}
    };
}

void StyleResolver::parseStylesheet(const std::string& css) {
    std::regex rulePattern(R"(([^{]+)\s*\{([^}]*)\})");
    std::regex declarationPattern(R"(([^:]+):\s*([^;]+);?)");
    
    auto begin = std::sregex_iterator(css.begin(), css.end(), rulePattern);
    auto end = std::sregex_iterator();
    
    for (auto it = begin; it != end; ++it) {
        std::string selectors = it->str(1);
        std::string declarations = it->str(2);
        
        // Split multiple selectors
        std::regex selectorSplit(R"(\s*,\s*)");
        auto selBegin = std::sregex_token_iterator(selectors.begin(), selectors.end(), selectorSplit, -1);
        auto selEnd = std::sregex_token_iterator();
        
        for (auto selIt = selBegin; selIt != selEnd; ++selIt) {
            std::string selector = *selIt;
            std::unordered_map<std::string, std::string> styleMap;
            
            // Parse declarations
            auto decBegin = std::sregex_iterator(declarations.begin(), declarations.end(), declarationPattern);
            auto decEnd = std::sregex_iterator();
            for (auto decIt = decBegin; decIt != decEnd; ++decIt) {
                std::string property = decIt->str(1);
                std::string value = decIt->str(2);
                // Trim whitespace
                property.erase(0, property.find_first_not_of(" \t"));
                property.erase(property.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                styleMap[property] = value;
            }
            
            rules_.push_back({selector, styleMap});
        }
    }
}

std::unordered_map<std::string, std::string> StyleResolver::resolveStyles(Element* element) {
    if (!element) return {};
    
    std::unordered_map<std::string, std::string> styles;
    
    // Start with user agent styles
    auto uaIt = userAgentStylesheet_.find(element->getTagName());
    if (uaIt != userAgentStylesheet_.end()) {
        styles.insert(uaIt->second.begin(), uaIt->second.end());
    }
    
    // Apply author styles (from parsed CSS)
    for (const auto& rule : rules_) {
        if (matchesSelector(element, rule.selector)) {
            int specificity = calculateSpecificity(rule.selector);
            // Higher specificity wins
            for (const auto& [prop, value] : rule.declarations) {
                styles[prop] = value;
            }
        }
    }
    
    // Inline styles have highest priority
    for (const auto& [prop, value] : element->getAllStyles()) {
        styles[prop] = value;
    }
    
    // Handle inheritance
    if (element->getParent()) {
        auto parentStyles = resolveStyles(static_cast<Element*>(element->getParent()));
        for (const auto& [prop, value] : parentStyles) {
            if (styles.find(prop) == styles.end() && isInheritedProperty(prop)) {
                styles[prop] = value;
            }
        }
    }
    
    return styles;
}

bool StyleResolver::matchesSelector(Element* element, const std::string& selector) {
    // Trim whitespace
    std::string sel = selector;
    sel.erase(0, sel.find_first_not_of(" \t"));
    sel.erase(sel.find_last_not_of(" \t") + 1);
    
    if (sel.empty()) return false;
    
    // ID selector: #id
    if (sel[0] == '#') {
        return element->getId() == sel.substr(1);
    }
    
    // Class selector: .class
    if (sel[0] == '.') {
        return element->hasClass(sel.substr(1));
    }
    
    // Tag selector
    if (sel.find_first_of(".#:") == std::string::npos) {
        return element->getTagName() == sel;
    }
    
    // Complex selectors: div.class, #id.class, etc.
    std::regex pattern(R"(([a-zA-Z0-9_-]+)?([.#][a-zA-Z0-9_-]+)*)");
    std::smatch match;
    if (std::regex_match(sel, match, pattern)) {
        // Tag part
        if (match[1].matched && match[1].str() != element->getTagName()) {
            return false;
        }
        
        // Class and ID parts
        std::string remaining = sel;
        size_t pos = 0;
        while ((pos = remaining.find_first_of(".#")) != std::string::npos) {
            char type = remaining[pos];
            size_t end = remaining.find_first_of(".#", pos + 1);
            std::string name = remaining.substr(pos + 1, end - pos - 1);
            
            if (type == '#') {
                if (element->getId() != name) return false;
            } else if (type == '.') {
                if (!element->hasClass(name)) return false;
            }
            
            remaining = remaining.substr(end != std::string::npos ? end : remaining.length());
        }
        return true;
    }
    
    return false;
}

int StyleResolver::calculateSpecificity(const std::string& selector) {
    int idCount = 0;
    int classCount = 0;
    int tagCount = 0;
    
    // Count ID selectors
    std::regex idPattern(R"(#[a-zA-Z0-9_-]+)");
    auto idBegin = std::sregex_iterator(selector.begin(), selector.end(), idPattern);
    auto idEnd = std::sregex_iterator();
    idCount = std::distance(idBegin, idEnd);
    
    // Count class selectors
    std::regex classPattern(R"(\.[a-zA-Z0-9_-]+)");
    auto classBegin = std::sregex_iterator(selector.begin(), selector.end(), classPattern);
    auto classEnd = std::sregex_iterator();
    classCount = std::distance(classBegin, classEnd);
    
    // Count tag selectors (and pseudo-elements)
    // Remove classes and IDs first
    std::string cleaned = std::regex_replace(selector, std::regex(R"([.#][a-zA-Z0-9_-]+)"), "");
    std::regex tagPattern(R"([a-zA-Z0-9_-]+)");
    auto tagBegin = std::sregex_iterator(cleaned.begin(), cleaned.end(), tagPattern);
    auto tagEnd = std::sregex_iterator();
    tagCount = std::distance(tagBegin, tagEnd);
    
    // Specificity: (idCount * 1000) + (classCount * 100) + (tagCount * 10)
    // Simplified for this implementation
    return idCount * 1000 + classCount * 100 + tagCount * 10;
}

bool StyleResolver::isInheritedProperty(const std::string& property) {
    static const std::unordered_set<std::string> inherited = {
        "color", "font-family", "font-size", "font-style", "font-weight",
        "text-align", "text-decoration", "text-transform", "letter-spacing",
        "line-height", "word-spacing", "visibility", "cursor", "list-style"
    };
    return inherited.find(property) != inherited.end();
}

// ==================== Layout Engine Implementation ====================

LayoutEngine::LayoutEngine(int viewportWidth, int viewportHeight)
    : viewportWidth_(viewportWidth), viewportHeight_(viewportHeight) {
}

LayoutBox LayoutEngine::computeLayout(Node* root) {
    LayoutBox rootBox;
    rootBox.node = root;
    rootBox.x = 0;
    rootBox.y = 0;
    rootBox.width = viewportWidth_;
    rootBox.height = 0;
    
    computeBoxModel(rootBox);
    layoutChildren(&rootBox);
    
    return rootBox;
}

void LayoutEngine::computeBoxModel(LayoutBox& box) {
    if (!box.node) return;
    
    // Get computed styles (would use StyleResolver in real implementation)
    std::unordered_map<std::string, std::string> styles;
    
    // Get width from styles or use default
    int contentWidth = 0;
    int contentHeight = 0;
    int padding = 0;
    int margin = 0;
    int border = 0;
    
    // Apply box model
    box.width = contentWidth + padding + border;
    box.height = contentHeight + padding + border;
}

void LayoutEngine::layoutChildren(LayoutBox* parent) {
    if (!parent || !parent->node) return;
    
    int y = parent->y + 10; // Default offset
    int x = parent->x + 10;
    
    for (Node* child : parent->node->getChildren()) {
        LayoutBox childBox;
        childBox.node = child;
        childBox.x = x;
        childBox.y = y;
        childBox.width = 100; // Default width
        childBox.height = 20; // Default height
        
        // Recursively layout children
        layoutChildren(&childBox);
        
        parent->children.push_back(childBox);
        y += childBox.height + 10;
    }
}

} // namespace BMS