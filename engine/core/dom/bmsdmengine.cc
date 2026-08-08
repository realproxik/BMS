// bms_dom_engine.cpp - BMS DOM Engine Implementation
#include "bms_dom_engine.h"
#include <algorithm>
#include <regex>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <chrono>

namespace BMS {
namespace DOM {

// ============================================================================
// BMSNode Implementation
// ============================================================================

BMSNode::BMSNode(NodeType type, const std::string& name)
    : type_(type), nodeName_(name) {
}

BMSNode::~BMSNode() {
    // Clean up children
    for (auto& child : children_) {
        child->parent_ = nullptr;
    }
}

BMSNode* BMSNode::getOwnerDocument() const {
    if (type_ == NodeType::DOCUMENT) {
        return const_cast<BMSNode*>(this);
    }
    
    BMSNode* current = const_cast<BMSNode*>(this);
    while (current && current->ownerDocument_ == nullptr) {
        current = current->parent_;
    }
    
    return current ? current->ownerDocument_ : nullptr;
}

BMSNode* BMSNode::getRootNode() const {
    BMSNode* current = const_cast<BMSNode*>(this);
    while (current->parent_) {
        current = current->parent_;
    }
    return current;
}

BMSNode* BMSNode::getChildAt(size_t index) const {
    if (index >= children_.size()) return nullptr;
    return children_[index].get();
}

BMSNode* BMSNode::appendChild(std::unique_ptr<BMSNode> child) {
    if (!child) return nullptr;
    if (this == child.get()) return nullptr;
    
    // Remove from current parent
    if (child->parent_) {
        child->parent_->removeChild(child.get());
    }
    
    child->parent_ = this;
    
    if (lastChild_) {
        lastChild_->nextSibling_ = child.get();
        child->previousSibling_ = lastChild_;
        child->nextSibling_ = nullptr;
        lastChild_ = child.get();
    } else {
        firstChild_ = child.get();
        lastChild_ = child.get();
        child->previousSibling_ = nullptr;
        child->nextSibling_ = nullptr;
    }
    
    children_.push_back(std::move(child));
    rebuildSearchIndex();
    return children_.back().get();
}

BMSNode* BMSNode::insertBefore(std::unique_ptr<BMSNode> newChild, BMSNode* refChild) {
    if (!newChild) return nullptr;
    if (refChild == nullptr) {
        return appendChild(std::move(newChild));
    }
    
    // Find refChild
    auto it = std::find_if(children_.begin(), children_.end(),
        [refChild](const std::unique_ptr<BMSNode>& child) {
            return child.get() == refChild;
        });
    
    if (it == children_.end()) return nullptr;
    if (this == newChild.get()) return nullptr;
    
    // Remove from current parent
    if (newChild->parent_) {
        newChild->parent_->removeChild(newChild.get());
    }
    
    newChild->parent_ = this;
    newChild->previousSibling_ = refChild->previousSibling_;
    newChild->nextSibling_ = refChild;
    
    if (refChild->previousSibling_) {
        refChild->previousSibling_->nextSibling_ = newChild.get();
    } else {
        firstChild_ = newChild.get();
    }
    
    refChild->previousSibling_ = newChild.get();
    
    children_.insert(it, std::move(newChild));
    rebuildSearchIndex();
    return children_.back().get();
}

BMSNode* BMSNode::replaceChild(std::unique_ptr<BMSNode> newChild, BMSNode* oldChild) {
    if (!newChild || !oldChild) return nullptr;
    
    auto it = std::find_if(children_.begin(), children_.end(),
        [oldChild](const std::unique_ptr<BMSNode>& child) {
            return child.get() == oldChild;
        });
    
    if (it == children_.end()) return nullptr;
    
    BMSNode* inserted = insertBefore(std::move(newChild), oldChild);
    if (!inserted) return nullptr;
    
    removeChild(oldChild);
    rebuildSearchIndex();
    return oldChild;
}

BMSNode* BMSNode::removeChild(BMSNode* child) {
    if (!child) return nullptr;
    
    auto it = std::find_if(children_.begin(), children_.end(),
        [child](const std::unique_ptr<BMSNode>& c) {
            return c.get() == child;
        });
    
    if (it == children_.end()) return nullptr;
    
    if (child->previousSibling_) {
        child->previousSibling_->nextSibling_ = child->nextSibling_;
    } else {
        firstChild_ = child->nextSibling_;
    }
    
    if (child->nextSibling_) {
        child->nextSibling_->previousSibling_ = child->previousSibling_;
    } else {
        lastChild_ = child->previousSibling_;
    }
    
    child->parent_ = nullptr;
    child->previousSibling_ = nullptr;
    child->nextSibling_ = nullptr;
    
    children_.erase(it);
    rebuildSearchIndex();
    return child;
}

void BMSNode::normalize() {
    std::vector<BMSNode*> toRemove;
    BMSNode* child = firstChild_;
    
    while (child && child->nextSibling_) {
        BMSNode* next = child->nextSibling_;
        
        if (child->type_ == NodeType::TEXT && next->type_ == NodeType::TEXT) {
            child->nodeValue_ += next->nodeValue_;
            toRemove.push_back(next);
            child->nextSibling_ = next->nextSibling_;
            if (next->nextSibling_) {
                next->nextSibling_->previousSibling_ = child;
            } else {
                lastChild_ = child;
            }
        } else {
            child->normalize();
            child = child->nextSibling_;
        }
    }
    
    for (BMSNode* node : toRemove) {
        removeChild(node);
    }
    rebuildSearchIndex();
}

std::unique_ptr<BMSNode> BMSNode::clone(bool deep) const {
    auto clone = std::make_unique<BMSNode>(type_, nodeName_);
    clone->nodeValue_ = nodeValue_;
    clone->namespaceURI_ = namespaceURI_;
    clone->prefix_ = prefix_;
    clone->localName_ = localName_;
    clone->customProperties_ = customProperties_;
    clone->searchIndex_ = searchIndex_;
    
    if (deep) {
        BMSNode* child = firstChild_;
        while (child) {
            auto childClone = child->clone(true);
            clone->appendChild(std::move(childClone));
            child = child->nextSibling_;
        }
    }
    
    return clone;
}

bool BMSNode::isEqual(const BMSNode* other) const {
    if (!other) return false;
    if (this == other) return true;
    if (type_ != other->type_) return false;
    if (nodeName_ != other->nodeName_) return false;
    if (nodeValue_ != other->nodeValue_) return false;
    if (children_.size() != other->children_.size()) return false;
    
    for (size_t i = 0; i < children_.size(); ++i) {
        if (!children_[i]->isEqual(other->children_[i].get())) {
            return false;
        }
    }
    return true;
}

std::string BMSNode::getTextContent() const {
    if (type_ == NodeType::TEXT || type_ == NodeType::CDATA_SECTION) {
        return nodeValue_;
    }
    
    if (type_ == NodeType::COMMENT) {
        return "";
    }
    
    std::stringstream ss;
    BMSNode* child = firstChild_;
    while (child) {
        ss << child->getTextContent();
        child = child->nextSibling_;
    }
    return ss.str();
}

void BMSNode::setTextContent(const std::string& text) {
    children_.clear();
    firstChild_ = nullptr;
    lastChild_ = nullptr;
    
    auto textNode = std::make_unique<BMSNode>(NodeType::TEXT, "#text");
    textNode->nodeValue_ = text;
    appendChild(std::move(textNode));
    rebuildSearchIndex();
}

std::string BMSNode::getOuterHTML() const {
    if (type_ == NodeType::TEXT || type_ == NodeType::CDATA_SECTION) {
        return nodeValue_;
    }
    
    if (type_ == NodeType::COMMENT) {
        return "<!--" + nodeValue_ + "-->";
    }
    
    if (type_ == NodeType::ELEMENT) {
        std::stringstream ss;
        ss << "<" << nodeName_;
        // Add attributes (if element)
        // This would be implemented in BMSElement
        ss << ">";
        ss << getInnerHTML();
        ss << "</" << nodeName_ << ">";
        return ss.str();
    }
    
    return "";
}

std::string BMSNode::getInnerHTML() const {
    std::stringstream ss;
    BMSNode* child = firstChild_;
    while (child) {
        ss << child->getOuterHTML();
        child = child->nextSibling_;
    }
    return ss.str();
}

std::string BMSNode::toXML(bool pretty) const {
    std::string xml = getOuterHTML();
    if (pretty) {
        // Simple pretty printing
        std::string result;
        int indent = 0;
        bool insideTag = false;
        for (char c : xml) {
            if (c == '<') {
                insideTag = true;
                if (!result.empty() && result.back() != '\n') {
                    result += '\n';
                }
                result += std::string(indent * 2, ' ');
                result += c;
            } else if (c == '>') {
                insideTag = false;
                result += c;
                if (xml.find("</") != std::string::npos) {
                    indent--;
                } else if (xml.find("/>") == std::string::npos) {
                    indent++;
                }
            } else if (c == '\n' || c == '\r') {
                // Skip
            } else {
                result += c;
            }
        }
        return result;
    }
    return xml;
}

uint16_t BMSNode::compareDocumentPosition(const BMSNode* other) const {
    if (!other) return DOCUMENT_POSITION_DISCONNECTED;
    if (this == other) return 0;
    
    BMSNode* root1 = getRootNode();
    BMSNode* root2 = other->getRootNode();
    
    if (root1 != root2) {
        return DOCUMENT_POSITION_DISCONNECTED | DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC;
    }
    
    if (contains(other)) {
        return DOCUMENT_POSITION_CONTAINS | DOCUMENT_POSITION_PRECEDING;
    }
    
    if (other->contains(this)) {
        return DOCUMENT_POSITION_CONTAINED_BY | DOCUMENT_POSITION_FOLLOWING;
    }
    
    return DOCUMENT_POSITION_FOLLOWING;
}

bool BMSNode::contains(const BMSNode* other) const {
    if (!other) return false;
    if (this == other) return true;
    
    BMSNode* current = const_cast<BMSNode*>(other);
    while (current->parent_) {
        if (current->parent_ == this) return true;
        current = current->parent_;
    }
    return false;
}

bool BMSNode::isAncestor(const BMSNode* other) const {
    return contains(other);
}

bool BMSNode::isDescendant(const BMSNode* other) const {
    return other ? other->contains(this) : false;
}

void BMSNode::render(void* context) const {
    // Platform-specific rendering
    // This would be implemented by the layout engine
}

void BMSNode::update(double deltaTime) {
    // Update node state
    for (auto& child : children_) {
        child->update(deltaTime);
    }
}

void BMSNode::onEvent(const std::string& event, void* data) {
    // Handle events
    for (auto& child : children_) {
        child->onEvent(event, data);
    }
}

void BMSNode::setCustomProperty(const std::string& key, const std::string& value) {
    customProperties_[key] = value;
}

std::string BMSNode::getCustomProperty(const std::string& key) const {
    auto it = customProperties_.find(key);
    return it != customProperties_.end() ? it->second : "";
}

void BMSNode::setSearchIndex(const std::string& index) {
    searchIndex_ = index;
}

std::string BMSNode::getSearchIndex() const {
    return searchIndex_;
}

void BMSNode::addToSearchIndex(const std::string& content) {
    searchIndex_ += " " + content;
}

void BMSNode::removeFromSearchIndex(const std::string& content) {
    size_t pos = searchIndex_.find(content);
    if (pos != std::string::npos) {
        searchIndex_.erase(pos, content.length());
    }
}

bool BMSNode::searchContent(const std::string& query) const {
    std::string normalizedQuery = query;
    std::transform(normalizedQuery.begin(), normalizedQuery.end(), 
                  normalizedQuery.begin(), ::tolower);
    
    std::string normalizedIndex = searchIndex_;
    std::transform(normalizedIndex.begin(), normalizedIndex.end(),
                  normalizedIndex.begin(), ::tolower);
    
    return normalizedIndex.find(normalizedQuery) != std::string::npos;
}

std::string BMSNode::toString() const {
    std::stringstream ss;
    ss << "Node{type=" << static_cast<int>(type_) 
       << ", name=" << nodeName_ 
       << ", value=" << nodeValue_ 
       << ", children=" << children_.size() << "}";
    return ss.str();
}

void BMSNode::print(int depth) const {
    for (int i = 0; i < depth; ++i) {
        std::cout << "  ";
    }
    
    const char* typeStr = "";
    switch (type_) {
        case NodeType::ELEMENT: typeStr = "ELEMENT"; break;
        case NodeType::TEXT: typeStr = "TEXT"; break;
        case NodeType::COMMENT: typeStr = "COMMENT"; break;
        case NodeType::DOCUMENT: typeStr = "DOCUMENT"; break;
        case NodeType::DOCUMENT_FRAGMENT: typeStr = "DOCUMENT_FRAGMENT"; break;
        default: typeStr = "UNKNOWN"; break;
    }
    
    std::cout << "[" << typeStr << "] " << nodeName_;
    if (!nodeValue_.empty()) {
        std::cout << " = \"" << nodeValue_ << "\"";
    }
    std::cout << std::endl;
    
    BMSNode* child = firstChild_;
    while (child) {
        child->print(depth + 1);
        child = child->nextSibling_;
    }
}

void BMSNode::printTree() const {
    print(0);
}

void BMSNode::updateChildPointers() {
    firstChild_ = nullptr;
    lastChild_ = nullptr;
    
    for (auto& child : children_) {
        child->parent_ = this;
        if (!firstChild_) {
            firstChild_ = child.get();
        }
        if (lastChild_) {
            lastChild_->nextSibling_ = child.get();
            child->previousSibling_ = lastChild_;
        }
        lastChild_ = child.get();
    }
}

void BMSNode::rebuildSearchIndex() {
    searchIndex_ = getTextContent();
    // Also index attributes if element
    if (type_ == NodeType::ELEMENT) {
        BMSElement* element = static_cast<BMSElement*>(this);
        for (const auto& [key, value] : element->getAllAttributes()) {
            searchIndex_ += " " + key + ":" + value;
        }
    }
}

// ============================================================================
// BMSNode Factory Methods
// ============================================================================

std::unique_ptr<BMSNode> BMSNode::createElement(const std::string& tagName) {
    return std::make_unique<BMSElement>(tagName);
}

std::unique_ptr<BMSNode> BMSNode::createTextNode(const std::string& text) {
    auto node = std::make_unique<BMSNode>(NodeType::TEXT, "#text");
    node->nodeValue_ = text;
    return node;
}

std::unique_ptr<BMSNode> BMSNode::createComment(const std::string& text) {
    auto node = std::make_unique<BMSNode>(NodeType::COMMENT, "#comment");
    node->nodeValue_ = text;
    return node;
}

std::unique_ptr<BMSNode> BMSNode::createDocument() {
    auto doc = std::make_unique<BMSDocument>();
    doc->ownerDocument_ = doc.get();
    return doc;
}

std::unique_ptr<BMSNode> BMSNode::createDocumentFragment() {
    return std::make_unique<BMSNode>(NodeType::DOCUMENT_FRAGMENT, "#document-fragment");
}

std::unique_ptr<BMSNode> BMSNode::createShadowRoot() {
    return std::make_unique<BMSNode>(NodeType::SHADOW_ROOT, "#shadow-root");
}

std::unique_ptr<BMSNode> BMSNode::createCustomElement(const std::string& name) {
    auto element = std::make_unique<BMSElement>(name);
    element->type_ = NodeType::CUSTOM_ELEMENT;
    return element;
}

// ============================================================================
// BMSElement Implementation
// ============================================================================

BMSElement::BMSElement(const std::string& tagName)
    : BMSNode(NodeType::ELEMENT, tagName) {
}

void BMSElement::setAttribute(const std::string& name, const std::string& value) {
    attributes_[name] = value;
    rebuildSearchIndex();
}

std::string BMSElement::getAttribute(const std::string& name) const {
    auto it = attributes_.find(name);
    return it != attributes_.end() ? it->second : "";
}

bool BMSElement::hasAttribute(const std::string& name) const {
    return attributes_.find(name) != attributes_.end();
}

void BMSElement::removeAttribute(const std::string& name) {
    attributes_.erase(name);
    rebuildSearchIndex();
}

std::unordered_map<std::string, std::string> BMSElement::getAllAttributes() const {
    return attributes_;
}

void BMSElement::setClassName(const std::string& className) {
    setAttribute("class", className);
}

std::string BMSElement::getClassName() const {
    return getAttribute("class");
}

bool BMSElement::hasClass(const std::string& className) const {
    std::string classStr = getAttribute("class");
    std::regex pattern("\\b" + className + "\\b");
    return std::regex_search(classStr, pattern);
}

void BMSElement::addClass(const std::string& className) {
    if (hasClass(className)) return;
    std::string classStr = getAttribute("class");
    if (!classStr.empty()) {
        classStr += " ";
    }
    classStr += className;
    setAttribute("class", classStr);
}

void BMSElement::removeClass(const std::string& className) {
    std::string classStr = getAttribute("class");
    std::regex pattern("\\b" + className + "\\b\\s*");
    std::string result = std::regex_replace(classStr, pattern, "");
    setAttribute("class", result);
}

void BMSElement::toggleClass(const std::string& className) {
    if (hasClass(className)) {
        removeClass(className);
    } else {
        addClass(className);
    }
}

void BMSElement::setId(const std::string& id) {
    setAttribute("id", id);
}

std::string BMSElement::getId() const {
    return getAttribute("id");
}

void BMSElement::setStyle(const std::string& property, const std::string& value) {
    styles_[property] = value;
    
    // Update style attribute
    std::stringstream ss;
    for (const auto& [prop, val] : styles_) {
        if (!ss.str().empty()) ss << "; ";
        ss << prop << ": " << val;
    }
    setAttribute("style", ss.str());
}

std::string BMSElement::getStyle(const std::string& property) const {
    auto it = styles_.find(property);
    return it != styles_.end() ? it->second : "";
}

std::unordered_map<std::string, std::string> BMSElement::getAllStyles() const {
    return styles_;
}

void BMSElement::addEventListener(const std::string& event, 
                                 std::function<void(void*)> callback) {
    eventListeners_[event] = callback;
}

void BMSElement::removeEventListener(const std::string& event) {
    eventListeners_.erase(event);
}

void BMSElement::dispatchEvent(const std::string& event, void* data) {
    auto it = eventListeners_.find(event);
    if (it != eventListeners_.end()) {
        it->second(data);
    }
    
    // Bubble up
    if (parent_) {
        parent_->onEvent(event, data);
    }
}

void BMSElement::render(void* context) const {
    // Render element
    for (const auto& child : children_) {
        child->render(context);
    }
}

void BMSElement::update(double deltaTime) {
    // Update element state
    BMSNode::update(deltaTime);
}

void BMSElement::onEvent(const std::string& event, void* data) {
    dispatchEvent(event, data);
}

std::unique_ptr<BMSNode> BMSElement::attachShadow() {
    shadowRoot_ = BMSNode::createShadowRoot();
    shadowRoot_->parent_ = this;
    return std::move(shadowRoot_);
}

void BMSElement::connectedCallback() {
    isConnected_ = true;
    // Called when element is added to document
}

void BMSElement::disconnectedCallback() {
    isConnected_ = false;
    // Called when element is removed from document
}

void BMSElement::adoptedCallback() {
    // Called when element is moved to a new document
}

void BMSElement::attributeChangedCallback(const std::string& name,
                                          const std::string& oldValue,
                                          const std::string& newValue) {
    // Called when an attribute changes
}

// ============================================================================
// BMSDocument Implementation
// ============================================================================

BMSDocument::BMSDocument()
    : BMSNode(NodeType::DOCUMENT, "#document") {
    ownerDocument_ = this;
}

std::string BMSDocument::getTitle() const {
    return title_;
}

void BMSDocument::setTitle(const std::string& title) {
    title_ = title;
    // Update title element if exists
    auto titleElements = getElementsByTagName("title");
    if (!titleElements.empty()) {
        titleElements[0]->setTextContent(title);
    }
}

BMSElement* BMSDocument::getDocumentElement() const {
    for (auto& child : children_) {
        if (child->getType() == NodeType::ELEMENT) {
            BMSElement* element = static_cast<BMSElement*>(child.get());
            if (element->getTagName() == "html") {
                return element;
            }
        }
    }
    return nullptr;
}

BMSElement* BMSDocument::getElementById(const std::string& id) const {
    return findElementById(const_cast<BMSNode*>(this), id);
}

BMSElement* BMSDocument::findElementById(BMSNode* node, const std::string& id) const {
    if (!node) return nullptr;
    
    if (node->getType() == NodeType::ELEMENT) {
        BMSElement* element = static_cast<BMSElement*>(node);
        if (element->getId() == id) {
            return element;
        }
    }
    
    for (auto& child : node->children_) {
        BMSElement* result = findElementById(child.get(), id);
        if (result) return result;
    }
    
    return nullptr;
}

std::vector<BMSElement*> BMSDocument::getElementsByTagName(const std::string& tagName) const {
    std::vector<BMSElement*> results;
    findElementsByTagName(const_cast<BMSNode*>(this), tagName, results);
    return results;
}

void BMSDocument::findElementsByTagName(BMSNode* node, const std::string& tagName,
                                       std::vector<BMSElement*>& results) const {
    if (!node) return;
    
    if (node->getType() == NodeType::ELEMENT) {
        BMSElement* element = static_cast<BMSElement*>(node);
        if (element->getTagName() == tagName || tagName == "*") {
            results.push_back(element);
        }
    }
    
    for (auto& child : node->children_) {
        findElementsByTagName(child.get(), tagName, results);
    }
}

std::vector<BMSElement*> BMSDocument::getElementsByClassName(const std::string& className) const {
    std::vector<BMSElement*> results;
    findElementsByClassName(const_cast<BMSNode*>(this), className, results);
    return results;
}

void BMSDocument::findElementsByClassName(BMSNode* node, const std::string& className,
                                         std::vector<BMSElement*>& results) const {
    if (!node) return;
    
    if (node->getType() == NodeType::ELEMENT) {
        BMSElement* element = static_cast<BMSElement*>(node);
        if (element->hasClass(className)) {
            results.push_back(element);
        }
    }
    
    for (auto& child : node->children_) {
        findElementsByClassName(child.get(), className, results);
    }
}

BMSElement* BMSDocument::querySelector(const std::string& selector) const {
    // Simple selector parsing
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

std::vector<BMSElement*> BMSDocument::querySelectorAll(const std::string& selector) const {
    if (selector.empty()) return {};
    
    if (selector[0] == '#') {
        auto element = getElementById(selector.substr(1));
        return element ? std::vector<BMSElement*>{element} : std::vector<BMSElement*>{};
    } else if (selector[0] == '.') {
        return getElementsByClassName(selector.substr(1));
    } else {
        return getElementsByTagName(selector);
    }
}

void BMSDocument::parseHTML(const std::string& html) {
    // Simple HTML parser
    // In production, this would use a proper HTML parser
    // For now, just create a basic structure
    children_.clear();
    firstChild_ = nullptr;
    lastChild_ = nullptr;
    
    // Create HTML element
    auto htmlElement = std::make_unique<BMSElement>("html");
    auto headElement = std::make_unique<BMSElement>("head");
    auto bodyElement = std::make_unique<BMSElement>("body");
    
    // Parse title
    std::regex titlePattern(R"(<title>([^<]*)</title>)");
    std::smatch titleMatch;
    if (std::regex_search(html, titleMatch, titlePattern)) {
        title_ = titleMatch[1];
        auto titleElement = std::make_unique<BMSElement>("title");
        titleElement->setTextContent(title_);
        headElement->appendChild(std::move(titleElement));
    }
    
    // Add basic structure
    htmlElement->appendChild(std::move(headElement));
    htmlElement->appendChild(std::move(bodyElement));
    appendChild(std::move(htmlElement));
    
    rebuildSearchIndex();
    indexDocument();
}

void BMSDocument::parseXML(const std::string& xml) {
    // XML parsing - similar to HTML but with stricter rules
    parseHTML(xml);
}

void BMSDocument::render(void* context) const {
    for (const auto& child : children_) {
        child->render(context);
    }
}

void BMSDocument::indexDocument() {
    std::lock_guard<std::mutex> lock(searchMutex_);
    
    std::string content = getTextContent();
    std::string url_ = url_;
    
    addToGlobalSearchIndex(url_, content);
}

std::vector<BMSNode*> BMSDocument::search(const std::string& query) const {
    std::vector<BMSNode*> results;
    
    // Search in this document
    std::function<void(BMSNode*)> searchNode = [&](BMSNode* node) {
        if (!node) return;
        
        if (node->searchContent(query)) {
            results.push_back(node);
        }
        
        for (auto& child : node->children_) {
            searchNode(child.get());
        }
    };
    
    searchNode(const_cast<BMSNode*>(static_cast<const BMSNode*>(this)));
    return results;
}

std::vector<BMSNode*> BMSDocument::searchGlobal(const std::string& query) const {
    std::vector<BMSNode*> results;
    
    // Search in global index
    std::lock_guard<std::mutex> lock(searchMutex_);
    
    for (const auto& [url, content] : globalSearchIndex_) {
        if (content.find(query) != std::string::npos) {
            // Create a reference node
            auto refNode = std::make_unique<BMSNode>(NodeType::ELEMENT, "a");
            refNode->setAttribute("href", url);
            refNode->setTextContent(url);
            results.push_back(refNode.get());
            // Store for later cleanup
        }
    }
    
    return results;
}

void BMSDocument::addToGlobalSearchIndex(const std::string& url, const std::string& content) {
    std::lock_guard<std::mutex> lock(searchMutex_);
    globalSearchIndex_[url] = content;
}

void BMSDocument::removeFromGlobalSearchIndex(const std::string& url) {
    std::lock_guard<std::mutex> lock(searchMutex_);
    globalSearchIndex_.erase(url);
}

// ============================================================================
// BMSSearchEngine Implementation
// ============================================================================

BMSSearchEngine* BMSSearchEngine::instance_ = nullptr;

BMSSearchEngine* BMSSearchEngine::getInstance() {
    if (!instance_) {
        instance_ = new BMSSearchEngine();
    }
    return instance_;
}

void BMSSearchEngine::indexPage(const std::string& url, const std::string& html) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    SearchEntry entry;
    entry.url = url;
    entry.title = "Page";
    entry.content = html;
    entry.lastVisited = std::chrono::system_clock::now();
    
    index_[url] = entry;
}

void BMSSearchEngine::indexPage(const std::string& url, const std::string& html, const std::string& title) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    SearchEntry entry;
    entry.url = url;
    entry.title = title;
    entry.content = html;
    entry.lastVisited = std::chrono::system_clock::now();
    
    index_[url] = entry;
}

void BMSSearchEngine::indexPage(const std::string& url, BMSDocument* document) {
    if (!document) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    SearchEntry entry;
    entry.url = url;
    entry.title = document->getTitle();
    entry.content = document->getTextContent();
    entry.lastVisited = std::chrono::system_clock::now();
    
    index_[url] = entry;
}

std::vector<std::pair<std::string, float>> BMSSearchEngine::search(const std::string& query, int maxResults) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<std::string, float>> results;
    
    // Normalize query
    std::string normalizedQuery = query;
    std::transform(normalizedQuery.begin(), normalizedQuery.end(), 
                  normalizedQuery.begin(), ::tolower);
    
    // Search in index
    for (const auto& [url, entry] : index_) {
        float relevance = calculateRelevance(entry.content, normalizedQuery);
        if (relevance > 0.0f) {
            results.push_back({url, relevance});
        }
    }
    
    // Sort by relevance
    std::sort(results.begin(), results.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
    
    // Limit results
    if (results.size() > static_cast<size_t>(maxResults)) {
        results.resize(maxResults);
    }
    
    return results;
}

std::vector<std::pair<std::string, float>> BMSSearchEngine::searchGlobal(const std::string& query, int maxResults) {
    // Global search across all websites
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<std::string, float>> results;
    
    // This would search across all indexed websites
    // For now, just search local index
    return search(query, maxResults);
}

void BMSSearchEngine::addWebsite(const std::string& url, const std::string& baseUrl) {
    std::lock_guard<std::mutex> lock(mutex_);
    websiteLinks_[baseUrl].push_back(url);
}

void BMSSearchEngine::removeWebsite(const std::string& url) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [baseUrl, links] : websiteLinks_) {
        auto it = std::remove(links.begin(), links.end(), url);
        if (it != links.end()) {
            links.erase(it, links.end());
        }
    }
}

void BMSSearchEngine::crawlWebsite(const std::string& url, int maxPages) {
    // Web crawler implementation
    // This would crawl the website and index all pages
    std::cout << "Crawling website: " << url << " (max pages: " << maxPages << ")" << std::endl;
    // Implementation omitted for brevity
}

void BMSSearchEngine::updateIndex(const std::string& url, const std::string& content) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = index_.find(url);
    if (it != index_.end()) {
        it->second.content = content;
        it->second.lastVisited = std::chrono::system_clock::now();
    }
}

void BMSSearchEngine::removeFromIndex(const std::string& url) {
    std::lock_guard<std::mutex> lock(mutex_);
    index_.erase(url);
}

void BMSSearchEngine::rebuildIndex() {
    std::lock_guard<std::mutex> lock(mutex_);
    // Rebuild index from all content
    // Implementation omitted for brevity
}

std::vector<std::string> BMSSearchEngine::getRelatedPages(const std::string& url, int maxResults) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> results;
    
    // Find pages related to the given URL
    auto it = index_.find(url);
    if (it != index_.end()) {
        // Search for similar content
        for (const auto& [otherUrl, entry] : index_) {
            if (otherUrl != url) {
                float similarity = calculateRelevance(it->second.content, entry.content);
                if (similarity > 0.5f) {
                    results.push_back(otherUrl);
                }
            }
        }
    }
    
    // Limit results
    if (results.size() > static_cast<size_t>(maxResults)) {
        results.resize(maxResults);
    }
    
    return results;
}

std::vector<std::string> BMSSearchEngine::getTopPages(int count) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<std::string, int>> pageVisits;
    
    for (const auto& [url, entry] : index_) {
        pageVisits.push_back({url, entry.visits});
    }
    
    std::sort(pageVisits.begin(), pageVisits.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
    
    std::vector<std::string> results;
    for (int i = 0; i < count && i < static_cast<int>(pageVisits.size()); ++i) {
        results.push_back(pageVisits[i].first);
    }
    
    return results;
}

float BMSSearchEngine::calculateRelevance(const std::string& content, const std::string& query) {
    // Simple relevance calculation
    std::string normalizedContent = content;
    std::string normalizedQuery = query;
    
    std::transform(normalizedContent.begin(), normalizedContent.end(),
                  normalizedContent.begin(), ::tolower);
    std::transform(normalizedQuery.begin(), normalizedQuery.end(),
                  normalizedQuery.begin(), ::tolower);
    
    // Count occurrences
    size_t pos = 0;
    int count = 0;
    while ((pos = normalizedContent.find(normalizedQuery, pos)) != std::string::npos) {
        ++count;
        pos += normalizedQuery.length();
    }
    
    if (count == 0) return 0.0f;
    
    // Normalize by content length
    float relevance = static_cast<float>(count) / (normalizedContent.length() / 100);
    return std::min(1.0f, relevance);
}

void BMSSearchEngine::tokenize(const std::string& text, std::vector<std::string>& tokens) {
    std::regex wordPattern(R"([a-zA-Z0-9_]+)");
    auto begin = std::sregex_iterator(text.begin(), text.end(), wordPattern);
    auto end = std::sregex_iterator();
    
    for (auto it = begin; it != end; ++it) {
        tokens.push_back(it->str());
    }
}

void BMSSearchEngine::stemWord(std::string& word) {
    // Simple stemming - remove common suffixes
    if (word.length() > 3) {
        if (word.find("ing") == word.length() - 3) {
            word = word.substr(0, word.length() - 3);
        } else if (word.find("ed") == word.length() - 2) {
            word = word.substr(0, word.length() - 2);
        } else if (word.find("ly") == word.length() - 2) {
            word = word.substr(0, word.length() - 2);
        } else if (word.find("es") == word.length() - 2) {
            word = word.substr(0, word.length() - 2);
        } else if (word.find("s") == word.length() - 1 && word.length() > 1) {
            word = word.substr(0, word.length() - 1);
        }
    }
}

void BMSSearchEngine::normalizeText(std::string& text) {
    std::transform(text.begin(), text.end(), text.begin(), ::tolower);
    // Remove extra spaces
    std::regex spacePattern(R"(\s+)");
    text = std::regex_replace(text, spacePattern, " ");
}

// ============================================================================
// BMSBrowserEngine Implementation
// ============================================================================

BMSBrowserEngine::BMSBrowserEngine() {
    searchEngine_ = std::make_unique<BMSSearchEngine>();
}

BMSBrowserEngine::~BMSBrowserEngine() {
    shutdown();
}

bool BMSBrowserEngine::initialize(int argc, char** argv) {
    if (initialized_) return true;
    
    // Initialize Chromium
    CefSettings settings;
    settings.multi_threaded_message_loop = true;
    
    // Initialize CEF
    CefInitialize(settings, nullptr, nullptr);
    
    initialized_ = true;
    return true;
}

void BMSBrowserEngine::shutdown() {
    if (!initialized_) return;
    
    // Close all windows
    for (auto& [handle, context] : windows_) {
        if (context->browser) {
            context->browser->CloseBrowser(true);
        }
    }
    windows_.clear();
    
    // Shutdown CEF
    CefShutdown();
    
    initialized_ = false;
}

void BMSBrowserEngine::createWindow(const std::string& url, int width, int height) {
    if (!initialized_) {
        std::cerr << "Browser engine not initialized" << std::endl;
        return;
    }
    
    // Create window context
    auto context = std::make_unique<WindowContext>();
    context->currentUrl = url;
    context->document = std::make_unique<BMSDocument>();
    
    // Store context
    void* handle = context.get();
    windows_[handle] = std::move(context);
    
    // Load URL
    navigate(handle, url);
}

void BMSBrowserEngine::closeWindow(void* windowHandle) {
    auto it = windows_.find(windowHandle);
    if (it != windows_.end()) {
        if (it->second->browser) {
            it->second->browser->CloseBrowser(true);
        }
        windows_.erase(it);
    }
}

void BMSBrowserEngine::navigate(void* windowHandle, const std::string& url) {
    auto it = windows_.find(windowHandle);
    if (it == windows_.end()) return;
    
    auto& context = it->second;
    context->currentUrl = url;
    
    // Add to history
    context->history.push_back(url);
    context->historyIndex = context->history.size() - 1;
    
    // Load in Chromium
    if (context->browser) {
        context->browser->GetMainFrame()->LoadURL(url);
    }
    
    // Load in BMS DOM
    // This would fetch the page and parse it
    // For now, just create a basic document
    context->document->setURL(url);
    
    // Index for search
    BMSSearchEngine::getInstance()->indexPage(url, context->document.get());
    
    // Notify
    onLoadStart(windowHandle);
}

void BMSBrowserEngine::refresh(void* windowHandle) {
    auto it = windows_.find(windowHandle);
    if (it == windows_.end()) return;
    
    if (it->second->browser) {
        it->second->browser->Reload();
    }
}

void BMSBrowserEngine::goBack(void* windowHandle) {
    auto it = windows_.find(windowHandle);
    if (it == windows_.end()) return;
    
    auto& context = it->second;
    if (context->historyIndex > 0) {
        context->historyIndex--;
        navigate(windowHandle, context->history[context->historyIndex]);
    }
}

void BMSBrowserEngine::goForward(void* windowHandle) {
    auto it = windows_.find(windowHandle);
    if (it == windows_.end()) return;
    
    auto& context = it->second;
    if (context->historyIndex < static_cast<int>(context->history.size()) - 1) {
        context->historyIndex++;
        navigate(windowHandle, context->history[context->historyIndex]);
    }
}

BMSDocument* BMSBrowserEngine::getDocument(void* windowHandle) const {
    auto it = windows_.find(windowHandle);
    if (it == windows_.end()) return nullptr;
    return it->second->document.get();
}

BMSElement* BMSBrowserEngine::getElementById(void* windowHandle, const std::string& id) const {
    auto doc = getDocument(windowHandle);
    return doc ? doc->getElementById(id) : nullptr;
}

std::vector<BMSElement*> BMSBrowserEngine::getElementsByTagName(void* windowHandle, 
                                                                const std::string& tagName) const {
    auto doc = getDocument(windowHandle);
    return doc ? doc->getElementsByTagName(tagName) : std::vector<BMSElement*>{};
}

void BMSBrowserEngine::search(const std::string& query) {
    // Search using default search engine
    auto results = searchEngine_->search(query);
    
    // Create search results page
    std::string html = "<html><head><title>Search Results</title></head><body>";
    html += "<h1>Search Results for: " + query + "</h1>";
    html += "<ul>";
    
    for (const auto& [url, relevance] : results) {
        html += "<li><a href=\"" + url + "\">" + url + "</a> (Relevance: " + 
                std::to_string(relevance) + ")</li>";
    }
    
    html += "</ul></body></html>";
    
    // Display results
    // This would create a new tab/window with search results
    std::cout << "Search results for: " << query << " (found " << results.size() << " results)" << std::endl;
}

void BMSBrowserEngine::searchGlobal(const std::string& query) {
    // Search across all websites
    auto results = searchEngine_->searchGlobal(query);
    
    // Display results
    std::cout << "Global search results for: " << query << " (found " << results.size() << " results)" << std::endl;
}

void BMSBrowserEngine::addSearchEngine(const std::string& name, const std::string& url) {
    // Add custom search engine
    std::cout << "Added search engine: " << name << " (" << url << ")" << std::endl;
}

void BMSBrowserEngine::setDefaultSearchEngine(const std::string& name) {
    std::cout << "Default search engine set to: " << name << std::endl;
}

std::vector<std::string> BMSBrowserEngine::getSearchEngines() const {
    return {"Google", "Bing", "DuckDuckGo", "BMS Search"};
}

void BMSBrowserEngine::loadIgnorePatterns(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Could not open ignore file: " << filePath << std::endl;
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;
        
        // Check if pattern is regex
        bool isRegex = false;
        if (line.find("*") != std::string::npos || 
            line.find("?") != std::string::npos ||
            line.find("[") != std::string::npos) {
            isRegex = true;
        }
        
        addIgnorePattern(line, isRegex);
    }
}

bool BMSBrowserEngine::shouldIgnore(const std::string& path) const {
    for (size_t i = 0; i < ignorePatterns_.size(); ++i) {
        bool match = false;
        if (ignorePatternIsRegex_[i]) {
            std::regex pattern(ignorePatterns_[i]);
            match = std::regex_search(path, pattern);
        } else {
            // Simple pattern matching with wildcards
            match = std::regex_search(path, std::regex(ignorePatterns_[i]));
        }
        if (match) return true;
    }
    return false;
}

void BMSBrowserEngine::addIgnorePattern(const std::string& pattern, bool isRegex) {
    ignorePatterns_.push_back(pattern);
    ignorePatternIsRegex_.push_back(isRegex);
}

void BMSBrowserEngine::removeIgnorePattern(const std::string& pattern) {
    for (auto it = ignorePatterns_.begin(); it != ignorePatterns_.end(); ++it) {
        if (*it == pattern) {
            size_t index = std::distance(ignorePatterns_.begin(), it);
            ignorePatterns_.erase(it);
            ignorePatternIsRegex_.erase(ignorePatternIsRegex_.begin() + index);
            break;
        }
    }
}

void BMSBrowserEngine::registerAPI(const std::string& name, 
                                   std::function<void(const std::string&)> handler) {
    apiHandlers_[name] = handler;
}

void BMSBrowserEngine::callAPI(const std::string& name, const std::string& data) {
    auto it = apiHandlers_.find(name);
    if (it != apiHandlers_.end()) {
        it->second(data);
    } else {
        std::cerr << "API not found: " << name << std::endl;
    }
}

void BMSBrowserEngine::registerAPIModule(const std::string& moduleName, void* module) {
    apiModules_[moduleName] = module;
}

CefRefPtr<CefBrowser> BMSBrowserEngine::getBrowser(void* windowHandle) const {
    auto it = windows_.find(windowHandle);
    if (it == windows_.end()) return nullptr;
    return it->second->browser;
}

void BMSBrowserEngine::injectJavaScript(void* windowHandle, const std::string& script) {
    auto browser = getBrowser(windowHandle);
    if (browser) {
        browser->GetMainFrame()->ExecuteJavaScript(script, "", 0);
    }
}

std::string BMSBrowserEngine::evaluateJavaScript(void* windowHandle, const std::string& script) {
    auto browser = getBrowser(windowHandle);
    if (!browser) return "";
    
    // This is simplified - in production would use CefV8Context
    return "JavaScript result";
}

void BMSBrowserEngine::onLoadStart(void* windowHandle) {
    std::cout << "Loading started" << std::endl;
}

void BMSBrowserEngine::onLoadEnd(void* windowHandle, int httpStatusCode) {
    std::cout << "Loading completed with status: " << httpStatusCode << std::endl;
}

void BMSBrowserEngine::onTitleChange(void* windowHandle, const std::string& title) {
    auto it = windows_.find(windowHandle);
    if (it != windows_.end()) {
        it->second->title = title;
    }
}

void BMSBrowserEngine::onAddressChange(void* windowHandle, const std::string& url) {
    auto it = windows_.find(windowHandle);
    if (it != windows_.end()) {
        it->second->currentUrl = url;
    }
}

void BMSBrowserEngine::onConsoleMessage(void* windowHandle, const std::string& message) {
    std::cout << "Console message: " << message << std::endl;
}

// ============================================================================
// BMSBrowserEngine::BMSBrowserClient Implementation
// ============================================================================

BMSBrowserEngine::BMSBrowserClient::BMSBrowserClient(BMSBrowserEngine* engine)
    : engine_(engine) {
}

// ============================================================================
// BMSLayoutEngine Implementation
// ============================================================================

BMSLayoutEngine::BMSLayoutEngine() {
}

BMSLayoutEngine::LayoutBox BMSLayoutEngine::computeLayout(BMSNode* root, 
                                                          int viewportWidth, 
                                                          int viewportHeight) {
    LayoutBox rootBox;
    rootBox.node = root;
    rootBox.x = 0;
    rootBox.y = 0;
    rootBox.width = viewportWidth;
    rootBox.height = viewportHeight;
    
    computeBoxModel(rootBox);
    computePosition(rootBox, 0, 0);
    
    // Layout children
    if (root) {
        BMSNode* child = root->getFirstChild();
        while (child) {
            LayoutBox childBox;
            childBox.node = child;
            childBox.x = rootBox.x + 10;
            childBox.y = rootBox.y + 10;
            childBox.width = rootBox.width - 20;
            childBox.height = 20;
            
            computeBoxModel(childBox);
            computePosition(childBox, rootBox.x, rootBox.y);
            
            // Recursively layout children
            LayoutBox grandChildBox = computeLayout(child, viewportWidth, viewportHeight);
            childBox.children.push_back(grandChildBox);
            
            rootBox.children.push_back(childBox);
            child = child->getNextSibling();
        }
    }
    
    return rootBox;
}

void BMSLayoutEngine::updateLayout(LayoutBox& box) {
    // Update layout tree
    for (auto& child : box.children) {
        updateLayout(child);
    }
}

void BMSLayoutEngine::renderLayout(const LayoutBox& box, void* context) {
    // Render layout box
    // Platform-specific rendering
    
    // Render children
    for (const auto& child : box.children) {
        renderLayout(child, context);
    }
}

void BMSLayoutEngine::setStyleResolver(
    std::function<std::unordered_map<std::string, std::string>(BMSElement*)> resolver) {
    styleResolver_ = resolver;
}

void BMSLayoutEngine::computeBoxModel(LayoutBox& box) {
    if (!box.node) return;
    
    // Compute box model from styles
    if (box.node->getType() == NodeType::ELEMENT) {
        BMSElement* element = static_cast<BMSElement*>(box.node);
        auto styles = styleResolver_ ? styleResolver_(element) : std::unordered_map<std::string, std::string>();
        box.computedStyles = styles;
        
        // Parse width, height, padding, margin, border
        auto widthIt = styles.find("width");
        if (widthIt != styles.end()) {
            box.width = std::stoi(widthIt->second);
        }
        
        auto heightIt = styles.find("height");
        if (heightIt != styles.end()) {
            box.height = std::stoi(heightIt->second);
        }
    }
}

void BMSLayoutEngine::computePosition(LayoutBox& box, int parentX, int parentY) {
    box.x += parentX;
    box.y += parentY;
}

void BMSLayoutEngine::computeFlexbox(LayoutBox& box) {
    // Implement flexbox layout
}

void BMSLayoutEngine::computeGrid(LayoutBox& box) {
    // Implement grid layout
}

void BMSLayoutEngine::computeBlockLayout(LayoutBox& box) {
    // Implement block layout
}

void BMSLayoutEngine::computeInlineLayout(LayoutBox& box) {
    // Implement inline layout
}

} // namespace DOM
} // namespace BMS