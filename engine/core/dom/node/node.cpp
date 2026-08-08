// node.cpp - DOM Node Implementation (C++)
#include "node.h"
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <sstream>

namespace BMS {
namespace DOM {

// ============================================================================
// Node Class Implementation
// ============================================================================

class NodeImpl {
public:
    NodeType type;
    std::string nodeName;
    std::string nodeValue;
    std::string namespaceURI;
    std::string prefix;
    std::string localName;
    
    NodeImpl* parentNode = nullptr;
    NodeImpl* firstChild = nullptr;
    NodeImpl* lastChild = nullptr;
    NodeImpl* previousSibling = nullptr;
    NodeImpl* nextSibling = nullptr;
    NodeImpl* ownerDocument = nullptr;
    
    void* userData = nullptr;
    std::vector<std::unique_ptr<NodeImpl>> children;
    
    NodeImpl(NodeType t, const std::string& name) 
        : type(t), nodeName(name) {}
    
    ~NodeImpl() {
        // Children are automatically destroyed by unique_ptr
    }
    
    void appendChild(std::unique_ptr<NodeImpl> child) {
        if (!child) return;
        
        child->parentNode = this;
        
        if (lastChild) {
            lastChild->nextSibling = child.get();
            child->previousSibling = lastChild;
            child->nextSibling = nullptr;
            lastChild = child.get();
        } else {
            firstChild = child.get();
            lastChild = child.get();
            child->previousSibling = nullptr;
            child->nextSibling = nullptr;
        }
        
        children.push_back(std::move(child));
    }
    
    void insertBefore(std::unique_ptr<NodeImpl> newChild, NodeImpl* refChild) {
        if (!newChild || !refChild) return;
        
        auto it = std::find_if(children.begin(), children.end(),
            [refChild](const std::unique_ptr<NodeImpl>& child) {
                return child.get() == refChild;
            });
        
        if (it == children.end()) return;
        
        newChild->parentNode = this;
        newChild->previousSibling = refChild->previousSibling;
        newChild->nextSibling = refChild;
        
        if (refChild->previousSibling) {
            refChild->previousSibling->nextSibling = newChild.get();
        } else {
            firstChild = newChild.get();
        }
        
        refChild->previousSibling = newChild.get();
        children.insert(it, std::move(newChild));
    }
    
    void removeChild(NodeImpl* child) {
        auto it = std::find_if(children.begin(), children.end(),
            [child](const std::unique_ptr<NodeImpl>& c) {
                return c.get() == child;
            });
        
        if (it == children.end()) return;
        
        if (child->previousSibling) {
            child->previousSibling->nextSibling = child->nextSibling;
        } else {
            firstChild = child->nextSibling;
        }
        
        if (child->nextSibling) {
            child->nextSibling->previousSibling = child->previousSibling;
        } else {
            lastChild = child->previousSibling;
        }
        
        child->parentNode = nullptr;
        child->previousSibling = nullptr;
        child->nextSibling = nullptr;
        
        children.erase(it);
    }
    
    NodeImpl* clone(bool deep) const {
        auto clone = std::make_unique<NodeImpl>(type, nodeName);
        clone->nodeValue = nodeValue;
        clone->namespaceURI = namespaceURI;
        clone->prefix = prefix;
        clone->localName = localName;
        clone->userData = userData;
        
        if (deep) {
            NodeImpl* child = firstChild;
            while (child) {
                auto childClone = std::unique_ptr<NodeImpl>(child->clone(true));
                clone->appendChild(std::move(childClone));
                child = child->nextSibling;
            }
        }
        
        return clone.release();
    }
    
    std::string getTextContent() const {
        if (type == NODE_TYPE_TEXT) {
            return nodeValue;
        }
        
        if (type == NODE_TYPE_COMMENT) {
            return "";
        }
        
        std::stringstream ss;
        NodeImpl* child = firstChild;
        while (child) {
            ss << child->getTextContent();
            child = child->nextSibling;
        }
        return ss.str();
    }
    
    void setTextContent(const std::string& text) {
        children.clear();
        firstChild = nullptr;
        lastChild = nullptr;
        
        auto textNode = std::make_unique<NodeImpl>(NODE_TYPE_TEXT, "#text");
        textNode->nodeValue = text;
        appendChild(std::move(textNode));
    }
    
    int getChildCount() const {
        return static_cast<int>(children.size());
    }
    
    NodeImpl* getChildAt(int index) const {
        if (index < 0 || index >= static_cast<int>(children.size())) {
            return nullptr;
        }
        return children[index].get();
    }
    
    std::string toString() const {
        std::stringstream ss;
        ss << "Node{type=" << type 
           << ", name=" << nodeName 
           << ", value=" << nodeValue 
           << ", children=" << children.size() << "}";
        return ss.str();
    }
};

// ============================================================================
// Public C++ API
// ============================================================================

Node::Node(NodeType type, const std::string& name) 
    : impl_(std::make_unique<NodeImpl>(type, name)) {}

Node::~Node() = default;

NodeType Node::getType() const { return impl_->type; }

std::string Node::getNodeName() const { return impl_->nodeName; }
void Node::setNodeName(const std::string& name) { impl_->nodeName = name; }

std::string Node::getNodeValue() const { return impl_->nodeValue; }
void Node::setNodeValue(const std::string& value) { impl_->nodeValue = value; }

std::string Node::getNamespaceURI() const { return impl_->namespaceURI; }
void Node::setNamespaceURI(const std::string& ns) { impl_->namespaceURI = ns; }

std::string Node::getPrefix() const { return impl_->prefix; }
void Node::setPrefix(const std::string& prefix) { impl_->prefix = prefix; }

std::string Node::getLocalName() const { return impl_->localName; }
void Node::setLocalName(const std::string& localName) { impl_->localName = localName; }

Node* Node::getParent() const {
    return impl_->parentNode ? new Node(impl_->parentNode) : nullptr;
}

Node* Node::getFirstChild() const {
    return impl_->firstChild ? new Node(impl_->firstChild) : nullptr;
}

Node* Node::getLastChild() const {
    return impl_->lastChild ? new Node(impl_->lastChild) : nullptr;
}

Node* Node::getPreviousSibling() const {
    return impl_->previousSibling ? new Node(impl_->previousSibling) : nullptr;
}

Node* Node::getNextSibling() const {
    return impl_->nextSibling ? new Node(impl_->nextSibling) : nullptr;
}

Node* Node::getOwnerDocument() const {
    return impl_->ownerDocument ? new Node(impl_->ownerDocument) : nullptr;
}

Node* Node::getRootNode() const {
    NodeImpl* current = impl_.get();
    while (current->parentNode) {
        current = current->parentNode;
    }
    return current != impl_.get() ? new Node(current) : nullptr;
}

int Node::getChildCount() const {
    return impl_->getChildCount();
}

Node* Node::getChildAt(int index) const {
    NodeImpl* child = impl_->getChildAt(index);
    return child ? new Node(child) : nullptr;
}

bool Node::hasChildNodes() const {
    return impl_->getChildCount() > 0;
}

void Node::appendChild(std::unique_ptr<Node> child) {
    if (!child) return;
    impl_->appendChild(std::move(child->impl_));
}

void Node::insertBefore(std::unique_ptr<Node> newChild, Node* refChild) {
    if (!newChild || !refChild) return;
    impl_->insertBefore(std::move(newChild->impl_), refChild->impl_.get());
}

void Node::replaceChild(std::unique_ptr<Node> newChild, Node* oldChild) {
    if (!newChild || !oldChild) return;
    // In real implementation, replace the child
}

void Node::removeChild(Node* child) {
    if (!child) return;
    impl_->removeChild(child->impl_.get());
}

void Node::normalize() {
    // Merge adjacent text nodes
    std::vector<NodeImpl*> toRemove;
    NodeImpl* child = impl_->firstChild;
    
    while (child && child->nextSibling) {
        NodeImpl* next = child->nextSibling;
        
        if (child->type == NODE_TYPE_TEXT && next->type == NODE_TYPE_TEXT) {
            child->nodeValue += next->nodeValue;
            toRemove.push_back(next);
            child->nextSibling = next->nextSibling;
            if (next->nextSibling) {
                next->nextSibling->previousSibling = child;
            } else {
                impl_->lastChild = child;
            }
        } else {
            // Normalize child
            // Recursive normalization would go here
            child = child->nextSibling;
        }
    }
    
    // Remove collected nodes
    for (NodeImpl* node : toRemove) {
        auto it = std::find_if(impl_->children.begin(), impl_->children.end(),
            [node](const std::unique_ptr<NodeImpl>& n) {
                return n.get() == node;
            });
        if (it != impl_->children.end()) {
            impl_->children.erase(it);
        }
    }
}

std::unique_ptr<Node> Node::clone(bool deep) const {
    return std::make_unique<Node>(impl_->clone(deep));
}

bool Node::isEqual(const Node* other) const {
    if (!other) return false;
    if (this == other) return true;
    if (impl_->type != other->impl_->type) return false;
    if (impl_->nodeName != other->impl_->nodeName) return false;
    if (impl_->nodeValue != other->impl_->nodeValue) return false;
    
    if (impl_->getChildCount() != other->impl_->getChildCount()) return false;
    
    for (int i = 0; i < impl_->getChildCount(); i++) {
        Node* child1 = getChildAt(i);
        Node* child2 = other->getChildAt(i);
        if (!child1->isEqual(child2)) return false;
    }
    
    return true;
}

std::string Node::getTextContent() const {
    return impl_->getTextContent();
}

void Node::setTextContent(const std::string& text) {
    impl_->setTextContent(text);
}

std::string Node::getOuterHTML() const {
    std::stringstream ss;
    
    if (impl_->type == NODE_TYPE_TEXT) {
        return impl_->nodeValue;
    }
    
if (impl_->type == NODE_TYPE_COMMENT) {
        ss << "<!--" << impl_->nodeValue << "-->";
    } else if (impl_->type == NODE_TYPE_PROCESSING_INSTRUCTION) {
        ss << "<?" << impl_->nodeName << " " << impl_->nodeValue << "?>";
    } else if (impl_->type == NODE_TYPE_ELEMENT) {
        ss << "<" << impl_->nodeName << ">";
        
        NodeImpl* child = impl_->firstChild;
        while (child) {
            Node childNode(child);
            ss << childNode.getOuterHTML();
            child = child->nextSibling;
        }
        
        ss << "</" << impl_->nodeName << ">";
    }
    
    return ss.str();
}

std::string Node::getInnerHTML() const {
    std::stringstream ss;
    
    NodeImpl* child = impl_->firstChild;
    while (child) {
        Node childNode(child);
        ss << childNode.getOuterHTML();
        child = child->nextSibling;
    }
    
    return ss.str();
}

unsigned short Node::compareDocumentPosition(const Node* other) const {
    if (!other) return DOCUMENT_POSITION_DISCONNECTED;
    if (this == other) return 0;
    
    // Simplified implementation
    return DOCUMENT_POSITION_FOLLOWING;
}

bool Node::contains(const Node* other) const {
    if (!other) return false;
    if (this == other) return true;
    
    const NodeImpl* current = other->impl_.get();
    while (current->parentNode) {
        if (current->parentNode == impl_.get()) return true;
        current = current->parentNode;
    }
    return false;
}

void Node::render(void* context) const {
    // Platform-specific rendering
    std::cout << "Rendering node: " << impl_->nodeName << std::endl;
}

std::string Node::toString() const {
    return impl_->toString();
}

std::string Node::toXML(bool pretty) const {
    // Simplified XML serialization
    if (pretty) {
        return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" + getOuterHTML();
    }
    return getOuterHTML();
}

void Node::print(int depth) const {
    for (int i = 0; i < depth; i++) {
        std::cout << "  ";
    }
    
    const char* typeStr = "";
    switch (impl_->type) {
        case NODE_TYPE_ELEMENT: typeStr = "ELEMENT"; break;
        case NODE_TYPE_TEXT: typeStr = "TEXT"; break;
        case NODE_TYPE_COMMENT: typeStr = "COMMENT"; break;
        case NODE_TYPE_PROCESSING_INSTRUCTION: typeStr = "PROCESSING_INSTRUCTION"; break;
        case NODE_TYPE_DOCUMENT: typeStr = "DOCUMENT"; break;
        default: typeStr = "UNKNOWN"; break;
    }
    
    std::cout << "[" << typeStr << "] " << impl_->nodeName;
    if (!impl_->nodeValue.empty()) {
        std::cout << " = \"" << impl_->nodeValue << "\"";
    }
    std::cout << std::endl;
    
    NodeImpl* child = impl_->firstChild;
    while (child) {
        Node childNode(child);
        childNode.print(depth + 1);
        child = child->nextSibling;
    }
}

void Node::printTree() const {
    print(0);
}

// ============================================================================
// Static Factory Methods
// ============================================================================

std::unique_ptr<Node> Node::createElement(const std::string& tagName) {
    return std::make_unique<Node>(NODE_TYPE_ELEMENT, tagName);
}

std::unique_ptr<Node> Node::createTextNode(const std::string& text) {
    auto node = std::make_unique<Node>(NODE_TYPE_TEXT, "#text");
    node->setNodeValue(text);
    return node;
}

std::unique_ptr<Node> Node::createComment(const std::string& text) {
    auto node = std::make_unique<Node>(NODE_TYPE_COMMENT, "#comment");
    node->setNodeValue(text);
    return node;
}

std::unique_ptr<Node> Node::createProcessingInstruction(const std::string& target, const std::string& data) {
    auto node = std::make_unique<Node>(NODE_TYPE_PROCESSING_INSTRUCTION, target);
    node->setNodeValue(data);
    return node;
}

std::unique_ptr<Node> Node::createDocument() {
    auto doc = std::make_unique<Node>(NODE_TYPE_DOCUMENT, "#document");
    doc->impl_->ownerDocument = doc->impl_.get();
    return doc;
}

std::unique_ptr<Node> Node::createDocumentFragment() {
    return std::make_unique<Node>(NODE_TYPE_DOCUMENT_FRAGMENT, "#document-fragment");
}

// ============================================================================
// Node Iterator Implementation
// ============================================================================

NodeIterator::NodeIterator(Node* root) : root_(root), current_(root) {}

NodeIterator::~NodeIterator() = default;

Node* NodeIterator::next() {
    if (!current_) {
        current_ = root_;
        return current_;
    }
    
    if (current_->getFirstChild()) {
        current_ = current_->getFirstChild();
        return current_;
    }
    
    if (current_->getNextSibling()) {
        current_ = current_->getNextSibling();
        return current_;
    }
    
    // Go up until we find a node with a next sibling
    Node* current = current_;
    while (current && current != root_) {
        if (current->getNextSibling()) {
            current_ = current->getNextSibling();
            return current_;
        }
        current = current->getParent();
    }
    
    current_ = nullptr;
    return nullptr;
}

Node* NodeIterator::previous() {
    if (!current_) return nullptr;
    
    if (current_->getPreviousSibling()) {
        current_ = current_->getPreviousSibling();
        while (current_->getLastChild()) {
            current_ = current_->getLastChild();
        }
        return current_;
    }
    
    if (current_->getParent()) {
        current_ = current_->getParent();
        return current_;
    }
    
    return nullptr;
}

} // namespace DOM
} // namespace BMS