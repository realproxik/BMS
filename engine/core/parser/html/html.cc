// engine/core/parser/html_parser.cpp (Continuation)
#include "html_parser.h"
#include <cctype>
#include <stack>
#include <algorithm>

namespace BMS {

// ==================== HTML Parser Implementation ====================

std::unique_ptr<Document> HTMLParser::parse(const std::string& html) {
    auto tokens = tokenize(html);
    return buildTree(tokens);
}

std::vector<HTMLParser::Token> HTMLParser::tokenize(const std::string& html) {
    std::vector<Token> tokens;
    size_t i = 0;
    size_t len = html.length();
    
    // State machine for tokenization
    enum State {
        TEXT,
        TAG,
        TAG_NAME,
        ATTRIBUTE_NAME,
        ATTRIBUTE_VALUE,
        COMMENT,
        DOCTYPE,
        SCRIPT
    };
    
    State state = TEXT;
    std::string current;
    std::string tagName;
    std::unordered_map<std::string, std::string> attributes;
    std::string attrName;
    std::string attrValue;
    char quote = '"';
    bool inQuotes = false;
    int scriptDepth = 0;
    
    while (i < len) {
        char c = html[i];
        
        switch (state) {
            case TEXT: {
                if (c == '<') {
                    // Check if it's a comment or doctype
                    if (i + 3 < len && html[i+1] == '!' && html[i+2] == '-' && html[i+3] == '-') {
                        // Comment start
                        if (!current.empty()) {
                            tokens.push_back({Token::TEXT, current, {}});
                            current.clear();
                        }
                        i += 4; // Skip <!--
                        state = COMMENT;
                    } else if (i + 1 < len && html[i+1] == '!') {
                        // Doctype
                        if (!current.empty()) {
                            tokens.push_back({Token::TEXT, current, {}});
                            current.clear();
                        }
                        i += 2; // Skip <!
                        state = DOCTYPE;
                    } else {
                        // Regular tag
                        if (!current.empty()) {
                            tokens.push_back({Token::TEXT, current, {}});
                            current.clear();
                        }
                        state = TAG;
                    }
                } else {
                    current += c;
                }
                break;
            }
            
            case TAG: {
                if (c == '/') {
                    // Closing tag
                    state = TAG_NAME;
                    tagName.clear();
                } else if (c == '!') {
                    // Comment or doctype
                    // ... (handled above)
                } else if (isalpha(c) || c == '_' || c == '-') {
                    tagName = c;
                    state = TAG_NAME;
                }
                break;
            }
            
            case TAG_NAME: {
                if (isspace(c)) {
                    if (!tagName.empty()) {
                        // Check if self-closing
                        bool selfClosing = false;
                        if (i + 1 < len && html[i+1] == '/') {
                            selfClosing = true;
                            i += 2; // Skip />
                        }
                        
                        Token token;
                        token.value = tagName;
                        token.attributes = attributes;
                        token.type = selfClosing ? Token::SELF_CLOSING_TAG : Token::START_TAG;
                        tokens.push_back(token);
                        
                        tagName.clear();
                        attributes.clear();
                        state = TEXT;
                    }
                    state = ATTRIBUTE_NAME;
                } else if (c == '>') {
                    // End of opening tag
                    Token token;
                    token.value = tagName;
                    token.attributes = attributes;
                    token.type = Token::START_TAG;
                    tokens.push_back(token);
                    
                    tagName.clear();
                    attributes.clear();
                    state = TEXT;
                } else if (c == '/') {
                    // Self-closing tag
                    if (i + 1 < len && html[i+1] == '>') {
                        Token token;
                        token.value = tagName;
                        token.attributes = attributes;
                        token.type = Token::SELF_CLOSING_TAG;
                        tokens.push_back(token);
                        
                        tagName.clear();
                        attributes.clear();
                        i += 1; // Skip >
                        state = TEXT;
                    }
                } else {
                    tagName += c;
                }
                break;
            }
            
            case ATTRIBUTE_NAME: {
                if (isspace(c)) {
                    if (!attrName.empty()) {
                        attributes[attrName] = "";
                        attrName.clear();
                    }
                } else if (c == '=') {
                    state = ATTRIBUTE_VALUE;
                    attrValue.clear();
                    quote = '"';
                    inQuotes = false;
                } else if (c == '>') {
                    if (!attrName.empty()) {
                        attributes[attrName] = "";
                        attrName.clear();
                    }
                    Token token;
                    token.value = tagName;
                    token.attributes = attributes;
                    token.type = Token::START_TAG;
                    tokens.push_back(token);
                    
                    tagName.clear();
                    attributes.clear();
                    state = TEXT;
                } else {
                    attrName += c;
                }
                break;
            }
            
            case ATTRIBUTE_VALUE: {
                if (inQuotes) {
                    if (c == quote) {
                        inQuotes = false;
                        attributes[attrName] = attrValue;
                        attrName.clear();
                        attrValue.clear();
                        state = ATTRIBUTE_NAME;
                    } else {
                        attrValue += c;
                    }
                } else {
                    if (c == '"' || c == '\'') {
                        quote = c;
                        inQuotes = true;
                    } else if (isspace(c)) {
                        if (!attrValue.empty()) {
                            attributes[attrName] = attrValue;
                            attrName.clear();
                            attrValue.clear();
                            state = ATTRIBUTE_NAME;
                        }
                    } else if (c == '>') {
                        if (!attrValue.empty()) {
                            attributes[attrName] = attrValue;
                            attrName.clear();
                            attrValue.clear();
                        }
                        Token token;
                        token.value = tagName;
                        token.attributes = attributes;
                        token.type = Token::START_TAG;
                        tokens.push_back(token);
                        
                        tagName.clear();
                        attributes.clear();
                        state = TEXT;
                    } else {
                        attrValue += c;
                    }
                }
                break;
            }
            
            case COMMENT: {
                if (c == '-' && i + 2 < len && html[i+1] == '-' && html[i+2] == '>') {
                    // End of comment
                    Token token;
                    token.type = Token::COMMENT;
                    token.value = current;
                    tokens.push_back(token);
                    
                    current.clear();
                    i += 2; // Skip -->
                    state = TEXT;
                } else {
                    current += c;
                }
                break;
            }
            
            case DOCTYPE: {
                if (c == '>') {
                    Token token;
                    token.type = Token::DOCTYPE;
                    token.value = current;
                    tokens.push_back(token);
                    
                    current.clear();
                    state = TEXT;
                } else {
                    current += c;
                }
                break;
            }
        }
        
        i++;
    }
    
    // Handle remaining text
    if (!current.empty()) {
        tokens.push_back({Token::TEXT, current, {}});
    }
    
    return tokens;
}

std::unique_ptr<Document> HTMLParser::buildTree(const std::vector<Token>& tokens) {
    auto doc = std::make_unique<Document>();
    std::stack<Element*> elementStack;
    Element* currentElement = nullptr;
    
    for (const auto& token : tokens) {
        switch (token.type) {
            case Token::START_TAG: {
                auto element = doc->createElement(token.value);
                for (const auto& [name, value] : token.attributes) {
                    element->setAttribute(name, value);
                }
                
                if (currentElement) {
                    currentElement->appendChild(std::unique_ptr<Node>(element));
                } else {
                    doc->appendChild(std::unique_ptr<Node>(element));
                }
                
                elementStack.push(element);
                currentElement = element;
                break;
            }
            
            case Token::END_TAG: {
                if (!elementStack.empty()) {
                    // Find matching opening tag
                    while (!elementStack.empty() && elementStack.top()->getTagName() != token.value) {
                        elementStack.pop();
                    }
                    if (!elementStack.empty()) {
                        elementStack.pop();
                    }
                    currentElement = elementStack.empty() ? nullptr : elementStack.top();
                }
                break;
            }
            
            case Token::SELF_CLOSING_TAG: {
                auto element = doc->createElement(token.value);
                for (const auto& [name, value] : token.attributes) {
                    element->setAttribute(name, value);
                }
                
                if (currentElement) {
                    currentElement->appendChild(std::unique_ptr<Node>(element));
                } else {
                    doc->appendChild(std::unique_ptr<Node>(element));
                }
                break;
            }
            
            case Token::TEXT: {
                if (currentElement) {
                    auto textNode = std::make_unique<TextNode>(token.value);
                    currentElement->appendChild(std::move(textNode));
                }
                break;
            }
            
            case Token::COMMENT: {
                auto comment = std::make_unique<CommentNode>(token.value);
                if (currentElement) {
                    currentElement->appendChild(std::move(comment));
                } else {
                    doc->appendChild(std::move(comment));
                }
                break;
            }
            
            case Token::DOCTYPE: {
                // Store doctype info
                break;
            }
        }
    }
    
    return doc;
}

} // namespace BMS