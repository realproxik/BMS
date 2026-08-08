// node.hh - DOM Node Header (C++ Alternative Style)
#ifndef BMS_NODE_HH
#define BMS_NODE_HH

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

namespace BMS {
namespace DOM {

// ============================================================================
// Type Definitions
// ============================================================================

enum class NodeType : uint8_t {
    ELEMENT = 1,
    ATTRIBUTE = 2,
    TEXT = 3,
    CDATA_SECTION = 4,
    PROCESSING_INSTRUCTION = 7,
    COMMENT = 8,
    DOCUMENT = 9,
    DOCUMENT_TYPE = 10,
    DOCUMENT_FRAGMENT = 11
};

enum class DocumentPosition : uint16_t {
    DISCONNECTED = 0x01,
    PRECEDING = 0x02,
    FOLLOWING = 0x04,
    CONTAINS = 0x08,
    CONTAINED_BY = 0x10,
    IMPLEMENTATION_SPECIFIC = 0x20
};

// ============================================================================
// Forward Declarations
// ============================================================================

class Node;
class Document;
class Element;
class Text;
class Comment;
class DocumentFragment;
class NodeIterator;
class TreeWalker;

// ============================================================================
// Node Class
// ============================================================================

class Node {
public:
    // Constructors
    Node(NodeType type, const std::string& name = "");
    virtual ~Node();
    
    // Copy and move
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&&) = default;
    Node& operator=(Node&&) = default;
    
    // Properties
    NodeType getType() const { return type_; }
    const std::string& getNodeName() const { return nodeName_; }
    void setNodeName(const std::string& name) { nodeName_ = name; }
    
    const std::string& getNodeValue() const { return nodeValue_; }
    void setNodeValue(const std::string& value) { nodeValue_ = value; }
    
    const std::string& getNamespaceURI() const { return namespaceURI_; }
    void setNamespaceURI(const std::string& ns) { namespaceURI_ = ns; }
    
    const std::string& getPrefix() const { return prefix_; }
    void setPrefix(const std::string& prefix) { prefix_ = prefix; }
    
    const std::string& getLocalName() const { return localName_; }
    void setLocalName(const std::string& localName) { localName_ = localName; }
    
    // Tree Navigation
    Node* getParent() const { return parent_; }
    Node* getFirstChild() const { return firstChild_; }
    Node* getLastChild() const { return lastChild_; }
    Node* getPreviousSibling() const { return previousSibling_; }
    Node* getNextSibling() const { return nextSibling_; }
    Node* getOwnerDocument() const;
    Node* getRootNode() const;
    
    // Child Management
    size_t getChildCount() const { return children_.size(); }
    Node* getChildAt(size_t index) const;
    bool hasChildNodes() const { return !children_.empty(); }
    bool hasChildren() const { return hasChildNodes(); }
    
    // Tree Manipulation
    Node* appendChild(std::unique_ptr<Node> child);
    Node* insertBefore(std::unique_ptr<Node> newChild, Node* refChild);
    Node* replaceChild(std::unique_ptr<Node> newChild, Node* oldChild);
    Node* removeChild(Node* child);
    void normalize();
    
    // Cloning
    std::unique_ptr<Node> clone(bool deep = true) const;
    bool isEqual(const Node* other) const;
    bool isSame(const Node* other) const { return this == other; }
    
    // Text Content
    std::string getTextContent() const;
    void setTextContent(const std::string& text);
    
    // HTML Serialization
    std::string getOuterHTML() const;
    std::string getInnerHTML() const;
    
    // Comparison
    uint16_t compareDocumentPosition(const Node* other) const;
    bool contains(const Node* other) const;
    bool isAncestor(const Node* other) const { return contains(other); }
    bool isDescendant(const Node* other) const;
    
    // Rendering
    virtual void render(void* context) const;
    
    // Debugging
    std::string toString() const;
    std::string toXML(bool pretty = true) const;
    void print(int depth = 0) const;
    void printTree() const;
    
    // User Data
    void* getUserData() const { return userData_; }
    void setUserData(void* data) { userData_ = data; }
    
    // Factory Methods
    static std::unique_ptr<Node> createElement(const std::string& tagName);
    static std::unique_ptr<Node> createTextNode(const std::string& text);
    static std::unique_ptr<Node> createComment(const std::string& text);
    static std::unique_ptr<Node> createDocument();
    static std::unique_ptr<Node> createDocumentFragment();
    
protected:
    NodeType type_;
    std::string nodeName_;
    std::string nodeValue_;
    std::string namespaceURI_;
    std::string prefix_;
    std::string localName_;
    
    Node* parent_ = nullptr;
    std::vector<std::unique_ptr<Node>> children_;
    Node* firstChild_ = nullptr;
    Node* lastChild_ = nullptr;
    Node* previousSibling_ = nullptr;
    Node* nextSibling_ = nullptr;
    Node* ownerDocument_ = nullptr;
    
    void* userData_ = nullptr;
    
private:
    void updateChildPointers();
};

// ============================================================================
// Specific Node Types
// ============================================================================

class Element : public Node {
public:
    explicit Element(const std::string& tagName);
    virtual ~Element() = default;
    
    std::string getTagName() const { return nodeName_; }
    void setAttribute(const std::string& name, const std::string& value);
    std::string getAttribute(const std::string& name) const;
    bool hasAttribute(const std::string& name) const;
    void removeAttribute(const std::string& name);
    
    void setClassName(const std::string& className);
    std::string getClassName() const;
    bool hasClass(const std::string& className) const;
    void addClass(const std::string& className);
    void removeClass(const std::string& className);
    void toggleClass(const std::string& className);
    
    void setId(const std::string& id);
    std::string getId() const;
    
    void render(void* context) const override;
};

class Text : public Node {
public:
    explicit Text(const std::string& text);
    virtual ~Text() = default;
    
    std::string getText() const { return nodeValue_; }
    void setText(const std::string& text) { nodeValue_ = text; }
    
    void render(void* context) const override;
};

class Comment : public Node {
public:
    explicit Comment(const std::string& text);
    virtual ~Comment() = default;
    
    std::string getComment() const { return nodeValue_; }
    void setComment(const std::string& text) { nodeValue_ = text; }
};

class Document : public Node {
public:
    Document();
    virtual ~Document() = default;
    
    std::string getTitle() const;
    void setTitle(const std::string& title);
    
    Element* getDocumentElement() const;
    Element* getElementById(const std::string& id) const;
    std::vector<Element*> getElementsByTagName(const std::string& tagName) const;
    std::vector<Element*> getElementsByClassName(const std::string& className) const;
};

class DocumentFragment : public Node {
public:
    DocumentFragment();
    virtual ~DocumentFragment() = default;
};

// ============================================================================
// Node Iterators
// ============================================================================

class NodeIterator {
public:
    explicit NodeIterator(Node* root);
    ~NodeIterator();
    
    Node* next();
    Node* previous();
    Node* getCurrent() const { return current_; }
    void reset() { current_ = root_; }
    
private:
    Node* root_;
    Node* current_;
};

class TreeWalker {
public:
    using FilterFunction = std::function<bool(Node*)>;
    
    TreeWalker(Node* root, uint16_t whatToShow = 0xFFFFFFFF);
    ~TreeWalker();
    
    Node* next();
    Node* previous();
    Node* parent();
    Node* firstChild();
    Node* lastChild();
    Node* previousSibling();
    Node* nextSibling();
    
    Node* getCurrent() const { return current_; }
    void setCurrent(Node* node) { current_ = node; }
    void setFilter(FilterFunction filter) { filter_ = filter; }
    
private:
    Node* root_;
    Node* current_;
    uint16_t whatToShow_;
    FilterFunction filter_;
};

} // namespace DOM
} // namespace BMS

#endif // BMS_NODE_HH