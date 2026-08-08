#include "node.h"
#include <algorithm>
#include <sstream>
#include <regex>
#include <iomanip>

namespace BMS {

// ==================== Node Implementation ====================

Node::Node(NodeType type, const std::string& nodeName)
    : type_(type), nodeName_(nodeName) {
    // Initialize node with basic properties
}

Node::~Node() {
    // Clean up children
    for (auto& child : children_) {
        child->parent_ = nullptr;
    }
}

void Node::appendChild(std::unique_ptr<Node> child) {
    if (child) {
        child->parent_ = this;
        children_.push_back(std::move(child));
    }
}

void Node::insertBefore(std::unique_ptr<Node> newNode, Node* refNode) {
    if (!newNode || !refNode) return;
    
    auto it = std::find_if(children_.begin(), children_.end(),
        [refNode](const std::unique_ptr<Node>& child) {
            return child.get() == refNode;
        });
    
    if (it != children_.end()) {
        newNode->parent_ = this;
        children_.insert(it, std::move(newNode));
    }
}

void Node::replaceChild(std::unique_ptr<Node> newNode, Node* oldNode) {
    if (!newNode || !oldNode) return;
    
    auto it = std::find_if(children_.begin(), children_.end(),
        [oldNode](const std::unique_ptr<Node>& child) {
            return child.get() == oldNode;
        });
    
    if (it != children_.end()) {
        newNode->parent_ = this;
        it->swap(newNode);
    }
}

void Node::removeChild(Node* child) {
    auto it = std::find_if(children_.begin(), children_.end(),
        [child](const std::unique_ptr<Node>& node) {
            return node.get() == child;
        });
    
    if (it != children_.end()) {
        (*it)->parent_ = nullptr;
        children_.erase(it);
    }
}

std::vector<Node*> Node::getChildren() const {
    std::vector<Node*> result;
    result.reserve(children_.size());
    for (const auto& child : children_) {
        result.push_back(child.get());
    }
    return result;
}

Node* Node::getFirstChild() const {
    return children_.empty() ? nullptr : children_.front().get();
}

Node* Node::getLastChild() const {
    return children_.empty() ? nullptr : children_.back().get();
}

Node* Node::getNextSibling() const {
    if (!parent_) return nullptr;
    
    const auto& siblings = parent_->children_;
    auto it = std::find_if(siblings.begin(), siblings.end(),
        [this](const std::unique_ptr<Node>& child) {
            return child.get() == this;
        });
    
    if (it != siblings.end() && ++it != siblings.end()) {
        return it->get();
    }
    return nullptr;
}

Node* Node::getPreviousSibling() const {
    if (!parent_) return nullptr;
    
    const auto& siblings = parent_->children_;
    auto it = std::find_if(siblings.begin(), siblings.end(),
        [this](const std::unique_ptr<Node>& child) {
            return child.get() == this;
        });
    
    if (it != siblings.begin()) {
        return (--it)->get();
    }
    return nullptr;
}

bool Node::hasChildNodes() const {
    return !children_.empty();
}

void Node::setAttribute(const std::string& name, const std::string& value) {
    attributes_[name] = value;
}

std::string Node::getAttribute(const std::string& name) const {
    auto it = attributes_.find(name);
    return it != attributes_.end() ? it->second : "";
}

bool Node::hasAttribute(const std::string& name) const {
    return attributes_.find(name) != attributes_.end();
}

void Node::removeAttribute(const std::string& name) {
    attributes_.erase(name);
}

std::unordered_map<std::string, std::string> Node::getAllAttributes() const {
    return attributes_;
}

std::string Node::getNodeValue() const {
    return nodeValue_;
}

void Node::setNodeValue(const std::string& value) {
    nodeValue_ = value;
}

Node* Node::cloneNode(bool deep) const {
    auto clone = std::make_unique<Node>(type_, nodeName_);
    clone->attributes_ = attributes_;
    clone->nodeValue_ = nodeValue_;
    
    if (deep) {
        for (const auto& child : children_) {
            clone->appendChild(std::unique_ptr<Node>(child->cloneNode(true)));
        }
    }
    
    return clone.release();
}

bool Node::isEqualNode(const Node* other) const {
    if (!other) return false;
    if (type_ != other->type_) return false;
    if (nodeName_ != other->nodeName_) return false;
    if (attributes_ != other->attributes_) return false;
    if (children_.size() != other->children_.size()) return false;
    
    for (size_t i = 0; i < children_.size(); ++i) {
        if (!children_[i]->isEqualNode(other->children_[i].get())) {
            return false;
        }
    }
    return true;
}

std::string Node::toString() const {
    std::stringstream ss;
    ss << "Node(" << nodeName_ << ", type=" << static_cast<int>(type_) << ")";
    return ss.str();
}

// ==================== Element Implementation ====================

Element::Element(const std::string& tagName)
    : Node(NodeType::ELEMENT, tagName), tagName_(tagName) {
    // Initialize element
}

void Element::setInnerHTML(const std::string& html) {
    // Parse HTML and replace children
    // This would call the HTML parser
    children_.clear();
    // Simplified: Just create text node for now
    auto textNode = std::make_unique<Node>(NodeType::TEXT, "#text");
    textNode->setNodeValue(html);
    textNode->parent_ = this;
    children_.push_back(std::move(textNode));
}

std::string Element::getInnerHTML() const {
    std::stringstream ss;
    for (const auto& child : children_) {
        if (child->getType() == NodeType::ELEMENT) {
            ss << static_cast<Element*>(child.get())->getOuterHTML();
        } else if (child->getType() == NodeType::TEXT) {
            ss << child->getNodeValue();
        }
    }
    return ss.str();
}

std::string Element::getOuterHTML() const {
    std::stringstream ss;
    ss << "<" << tagName_;
    
    for (const auto& [name, value] : attributes_) {
        ss << " " << name << "=\"" << value << "\"";
    }
    
    if (selfClosingTags_.find(tagName_) != selfClosingTags_.end()) {
        ss << " />";
    } else {
        ss << ">";
        ss << getInnerHTML();
        ss << "</" << tagName_ << ">";
    }
    
    return ss.str();
}

std::string Element::getTextContent() const {
    std::stringstream ss;
    for (const auto& child : children_) {
        if (child->getType() == NodeType::TEXT) {
            ss << child->getNodeValue();
        } else if (child->getType() == NodeType::ELEMENT) {
            ss << static_cast<Element*>(child.get())->getTextContent();
        }
    }
    return ss.str();
}

void Element::setTextContent(const std::string& text) {
    children_.clear();
    auto textNode = std::make_unique<Node>(NodeType::TEXT, "#text");
    textNode->setNodeValue(text);
    textNode->parent_ = this;
    children_.push_back(std::move(textNode));
}

void Element::setStyle(const std::string& property, const std::string& value) {
    styleMap_[property] = value;
    updateStyleAttribute();
}

std::string Element::getStyle(const std::string& property) const {
    auto it = styleMap_.find(property);
    return it != styleMap_.end() ? it->second : "";
}

std::unordered_map<std::string, std::string> Element::getAllStyles() const {
    return styleMap_;
}

void Element::updateStyleAttribute() {
    std::stringstream ss;
    for (const auto& [property, value] : styleMap_) {
        if (!ss.str().empty()) ss << "; ";
        ss << property << ": " << value;
    }
    setAttribute("style", ss.str());
}

bool Element::hasClass(const std::string& className) const {
    std::string classStr = getAttribute("class");
    std::regex pattern("\\b" + className + "\\b");
    return std::regex_search(classStr, pattern);
}

void Element::addClass(const std::string& className) {
    if (hasClass(className)) return;
    
    std::string classStr = getAttribute("class");
    if (!classStr.empty()) {
        classStr += " ";
    }
    classStr += className;
    setAttribute("class", classStr);
}

void Element::removeClass(const std::string& className) {
    std::string classStr = getAttribute("class");
    std::regex pattern("\\b" + className + "\\b\\s*");
    std::string result = std::regex_replace(classStr, pattern, "");
    setAttribute("class", result);
}

void Element::toggleClass(const std::string& className) {
    if (hasClass(className)) {
        removeClass(className);
    } else {
        addClass(className);
    }
}

void Element::setId(const std::string& id) {
    setAttribute("id", id);
}

std::string Element::getId() const {
    return getAttribute("id");
}

void Element::addEventListener(const std::string& eventType, 
                               std::function<void(Event*)> callback) {
    eventListeners_[eventType].push_back(callback);
}

void Element::dispatchEvent(Event* event) {
    event->setTarget(this);
    event->setCurrentTarget(this);
    
    // Call listeners for this element
    auto it = eventListeners_.find(event->getType());
    if (it != eventListeners_.end()) {
        for (const auto& callback : it->second) {
            callback(event);
        }
    }
    
    // Bubble up
    if (event->isBubbles() && parent_) {
        parent_->dispatchEvent(event);
    }
}

// ==================== Document Implementation ====================

Document::Document()
    : Node(NodeType::DOCUMENT, "#document") {
    createDefaultStyleSheet();
}

void Document::createDefaultStyleSheet() {
    // Default styles for common elements
    defaultStyles_ = {
        {"body", {{"margin", "8px"}, {"font-family", "Arial, sans-serif"}}},
        {"h1", {{"font-size", "2em"}, {"font-weight", "bold"}, {"margin", "0.67em 0"}}},
        {"h2", {{"font-size", "1.5em"}, {"font-weight", "bold"}, {"margin", "0.83em 0"}}},
        {"p", {{"margin", "1em 0"}}},
        {"div", {{"display", "block"}}},
        {"span", {{"display", "inline"}}},
        {"a", {{"color", "#0066cc"}, {"text-decoration", "underline"}}},
        {"img", {{"max-width", "100%"}}},
        {"ul", {{"margin", "1em 0"}, {"padding-left", "40px"}}},
        {"li", {{"display", "list-item"}}},
        {"table", {{"border-collapse", "collapse"}}},
        {"td", {{"border", "1px solid #ddd"}, {"padding", "8px"}}},
        {"th", {{"border", "1px solid #ddd"}, {"padding", "8px"}, {"font-weight", "bold"}}}
    };
}

Element* Document::createElement(const std::string& tagName) {
    auto element = std::make_unique<Element>(tagName);
    Element* ptr = element.get();
    
    // Apply default styles
    auto it = defaultStyles_.find(tagName);
    if (it != defaultStyles_.end()) {
        for (const auto& [prop, value] : it->second) {
            ptr->setStyle(prop, value);
        }
    }
    
    // Store for later use
    createdElements_.push_back(std::move(element));
    return ptr;
}

Element* Document::getElementById(const std::string& id) const {
    return findElementById(const_cast<Document*>(this), id);
}

Element* Document::findElementById(Node* node, const std::string& id) const {
    if (!node) return nullptr;
    
    if (node->getType() == NodeType::ELEMENT) {
        Element* element = static_cast<Element*>(node);
        if (element->getId() == id) {
            return element;
        }
    }
    
    for (Node* child : node->getChildren()) {
        Element* result = findElementById(child, id);
        if (result) return result;
    }
    
    return nullptr;
}

std::vector<Element*> Document::getElementsByTagName(const std::string& tagName) const {
    std::vector<Element*> results;
    findElementsByTagName(const_cast<Document*>(this), tagName, results);
    return results;
}

void Document::findElementsByTagName(Node* node, const std::string& tagName, 
                                     std::vector<Element*>& results) const {
    if (!node) return;
    
    if (node->getType() == NodeType::ELEMENT) {
        Element* element = static_cast<Element*>(node);
        if (element->getTagName() == tagName || tagName == "*") {
            results.push_back(element);
        }
    }
    
    for (Node* child : node->getChildren()) {
        findElementsByTagName(child, tagName, results);
    }
}

std::vector<Element*> Document::getElementsByClassName(const std::string& className) const {
    std::vector<Element*> results;
    findElementsByClassName(const_cast<Document*>(this), className, results);
    return results;
}

void Document::findElementsByClassName(Node* node, const std::string& className,
                                       std::vector<Element*>& results) const {
    if (!node) return;
    
    if (node->getType() == NodeType::ELEMENT) {
        Element* element = static_cast<Element*>(node);
        if (element->hasClass(className)) {
            results.push_back(element);
        }
    }
    
    for (Node* child : node->getChildren()) {
        findElementsByClassName(child, className, results);
    }
}

Element* Document::querySelector(const std::string& selector) const {
    // Simple selector parsing (just ID, class, or tag)
    if (selector.empty()) return nullptr;
    
    if (selector[0] == '#') {
        return getElementById(selector.substr(1));
    } else if (selector[0] == '.') {
        auto results = getElementsByClassName(selector.substr(1));
        return results.empty() ? nullptr : results[0];
    } else {
        auto results = getElementsByTagName(selector);
        return results.empty() ? nullptr : results[0];
    }
}

std::vector<Element*> Document::querySelectorAll(const std::string& selector) const {
    if (selector.empty()) return {};
    
    if (selector[0] == '#') {
        auto element = getElementById(selector.substr(1));
        return element ? std::vector<Element*>{element} : std::vector<Element*>{};
    } else if (selector[0] == '.') {
        return getElementsByClassName(selector.substr(1));
    } else {
        return getElementsByTagName(selector);
    }
}

Node* Document::importNode(Node* node, bool deep) {
    if (!node) return nullptr;
    return node->cloneNode(deep);
}

Element* Document::getDocumentElement() const {
    for (Node* child : getChildren()) {
        if (child->getType() == NodeType::ELEMENT) {
            Element* element = static_cast<Element*>(child);
            if (element->getTagName() == "html") {
                return element;
            }
        }
    }
    return nullptr;
}

void Document::setTitle(const std::string& title) {
    title_ = title;
    // Update <title> element if it exists
    auto titleElements = getElementsByTagName("title");
    if (!titleElements.empty()) {
        titleElements[0]->setTextContent(title);
    }
}

std::string Document::getTitle() const {
    auto titleElements = getElementsByTagName("title");
    if (!titleElements.empty()) {
        return titleElements[0]->getTextContent();
    }
    return title_;
}

std::string Document::getOuterHTML() const {
    std::stringstream ss;
    ss << "<!DOCTYPE html>\n";
    for (const auto& child : children_) {
        if (child->getType() == NodeType::ELEMENT) {
            ss << static_cast<Element*>(child.get())->getOuterHTML();
        }
    }
    return ss.str();
}

void Document::parseHTML(const std::string& html) {
    // This would call the HTML parser
    // For now, just create a simple structure
    children_.clear();
    
    auto htmlElement = std::make_unique<Element>("html");
    auto headElement = std::make_unique<Element>("head");
    auto bodyElement = std::make_unique<Element>("body");
    
    // Add some basic content
    auto titleElement = std::make_unique<Element>("title");
    titleElement->setTextContent("BMS Browser");
    headElement->appendChild(std::move(titleElement));
    
    htmlElement->appendChild(std::move(headElement));
    htmlElement->appendChild(std::move(bodyElement));
    
    htmlElement->parent_ = this;
    children_.push_back(std::move(htmlElement));
}

// Static member initialization
const std::unordered_set<std::string> Element::selfClosingTags_ = {
    "area", "base", "br", "col", "embed", "hr", "img", "input",
    "keygen", "link", "meta", "param", "source", "track", "wbr"
};

// ==================== Text Node Implementation ====================

TextNode::TextNode(const std::string& text)
    : Node(NodeType::TEXT, "#text") {
    setNodeValue(text);
}

std::string TextNode::getText() const {
    return getNodeValue();
}

void TextNode::setText(const std::string& text) {
    setNodeValue(text);
}

std::string TextNode::toString() const {
    return "TextNode(\"" + getNodeValue() + "\")";
}

// ==================== Comment Node Implementation ====================

CommentNode::CommentNode(const std::string& comment)
    : Node(NodeType::COMMENT, "#comment") {
    setNodeValue(comment);
}

std::string CommentNode::getComment() const {
    return getNodeValue();
}

std::string CommentNode::toString() const {
    return "CommentNode(\"" + getNodeValue() + "\")";
}

// ==================== Document Fragment Implementation ====================

DocumentFragment::DocumentFragment()
    : Node(NodeType::DOCUMENT, "#document-fragment") {
}

void DocumentFragment::appendHTML(const std::string& html) {
    // Parse HTML and append to fragment
    // Simplified implementation
    auto textNode = std::make_unique<TextNode>(html);
    appendChild(std::move(textNode));
}

std::string DocumentFragment::getHTML() const {
    std::stringstream ss;
    for (const auto& child : children_) {
        if (child->getType() == NodeType::ELEMENT) {
            ss << static_cast<Element*>(child.get())->getOuterHTML();
        } else if (child->getType() == NodeType::TEXT) {
            ss << child->getNodeValue();
        }
    }
    return ss.str();
}

} // namespace BMS