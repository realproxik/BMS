// node.hpp - DOM Node Header (Boost-style)
#ifndef BMS_NODE_HPP
#define BMS_NODE_HPP

#include <boost/smart_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/container/vector.hpp>
#include <boost/functional/hash.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace BMS {
namespace DOM {
namespace boost = ::boost;

// ============================================================================
// Type Definitions
// ============================================================================

enum class NodeType : unsigned char {
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

enum class DocumentPosition : unsigned short {
    DISCONNECTED = 0x01,
    PRECEDING = 0x02,
    FOLLOWING = 0x04,
    CONTAINS = 0x08,
    CONTAINED_BY = 0x10,
    IMPLEMENTATION_SPECIFIC = 0x20
};

// ============================================================================
// Node Class (Boost-style)
// ============================================================================

class Node : public boost::enable_shared_from_this<Node> {
public:
    // Type definitions
    typedef boost::shared_ptr<Node> Ptr;
    typedef boost::weak_ptr<Node> WeakPtr;
    typedef boost::container::vector<Ptr> ChildrenContainer;
    typedef boost::container::vector<Ptr>::iterator Iterator;
    typedef boost::container::vector<Ptr>::const_iterator ConstIterator;
    
    // Constructors
    explicit Node(NodeType type, const std::string& name = "");
    virtual ~Node();
    
    // Factory methods
    static Ptr create(NodeType type, const std::string& name = "");
    static Ptr createElement(const std::string& tagName);
    static Ptr createTextNode(const std::string& text);
    static Ptr createComment(const std::string& text);
    static Ptr createProcessingInstruction(const std::string& target, const std::string& data);
    static Ptr createDocument();
    static Ptr createDocumentFragment();

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
    Ptr getParent() const { return parent_.lock(); }
    Ptr getFirstChild() const;
    Ptr getLastChild() const;
    Ptr getPreviousSibling() const;
    Ptr getNextSibling() const;
    Ptr getOwnerDocument() const;
    Ptr getRootNode() const;
    
    // Child Management
    size_t getChildCount() const { return children_.size(); }
    bool hasChildNodes() const { return !children_.empty(); }
    bool empty() const { return children_.empty(); }
    
    Ptr getChildAt(size_t index) const;
    size_t getChildIndex(const Ptr& child) const;
    
    Iterator begin() { return children_.begin(); }
    Iterator end() { return children_.end(); }
    ConstIterator begin() const { return children_.begin(); }
    ConstIterator end() const { return children_.end(); }
    
    // Tree Manipulation
    Ptr appendChild(const Ptr& child);
    Ptr insertBefore(const Ptr& newChild, const Ptr& refChild);
    Ptr replaceChild(const Ptr& newChild, const Ptr& oldChild);
    Ptr removeChild(const Ptr& child);
    void normalize();
    
    // Cloning
    Ptr clone(bool deep = true) const;
    bool isEqual(const Ptr& other) const;
    bool isSame(const Ptr& other) const { return this == other.get(); }
    
    // Text Content
    std::string getTextContent() const;
    void setTextContent(const std::string& text);
    
    // HTML Serialization
    std::string getOuterHTML() const;
    std::string getInnerHTML() const;
    
    // Comparison
    unsigned short compareDocumentPosition(const Ptr& other) const;
    bool contains(const Ptr& other) const;
    bool isAncestor(const Ptr& other) const;
    bool isDescendant(const Ptr& other) const;
    
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
    
    // Hash
    std::size_t hash() const { return boost::hash_value(this); }
    
protected:
    NodeType type_;
    std::string nodeName_;
    std::string nodeValue_;
    std::string namespaceURI_;
    std::string prefix_;
    std::string localName_;
    
    WeakPtr parent_;
    ChildrenContainer children_;
    WeakPtr ownerDocument_;
    
    void* userData_ = nullptr;
    
private:
    void updateSiblings(const Ptr& child);
    void detach(const Ptr& child);
};

// ============================================================================
// Element Class (Boost-style)
// ============================================================================

class Element : public Node {
public:
    typedef boost::shared_ptr<Element> Ptr;
    
    explicit Element(const std::string& tagName);
    virtual ~Element() = default;
    
    static Ptr create(const std::string& tagName);
    
    std::string getTagName() const { return nodeName_; }
    
    void setAttribute(const std::string& name, const std::string& value);
    std::string getAttribute(const std::string& name) const;
    bool hasAttribute(const std::string& name) const;
    void removeAttribute(const std::string& name);
    std::unordered_map<std::string, std::string> getAllAttributes() const;
    
    void setClassName(const std::string& className);
    std::string getClassName() const;
    bool hasClass(const std::string& className) const;
    void addClass(const std::string& className);
    void removeClass(const std::string& className);
    void toggleClass(const std::string& className);
    
    void setId(const std::string& id);
    std::string getId() const;
    
    void setStyle(const std::string& property, const std::string& value);
    std::string getStyle(const std::string& property) const;
    
    void render(void* context) const override;
    
private:
    std::unordered_map<std::string, std::string> attributes_;
    std::unordered_map<std::string, std::string> styles_;
};

// ============================================================================
// Text Class (Boost-style)
// ============================================================================

class Text : public Node {
public:
    typedef boost::shared_ptr<Text> Ptr;
    
    explicit Text(const std::string& text);
    virtual ~Text() = default;
    
    static Ptr create(const std::string& text);
    
    std::string getText() const { return nodeValue_; }
    void setText(const std::string& text) { nodeValue_ = text; }
    
    void render(void* context) const override;
};

// ============================================================================
// Comment Class (Boost-style)
// ============================================================================

class Comment : public Node {
public:
    typedef boost::shared_ptr<Comment> Ptr;
    
    explicit Comment(const std::string& text);
    virtual ~Comment() = default;
    
    static Ptr create(const std::string& text);
    
    std::string getComment() const { return nodeValue_; }
    void setComment(const std::string& text) { nodeValue_ = text; }
};

// ============================================================================
// Document Class (Boost-style)
// ============================================================================

class Document : public Node {
public:
    typedef boost::shared_ptr<Document> Ptr;
    
    Document();
    virtual ~Document() = default;
    
    static Ptr create();
    
    std::string getTitle() const;
    void setTitle(const std::string& title);
    
    Element::Ptr getDocumentElement() const;
    Element::Ptr getElementById(const std::string& id) const;
    std::vector<Element::Ptr> getElementsByTagName(const std::string& tagName) const;
    std::vector<Element::Ptr> getElementsByClassName(const std::string& className) const;
};

// ============================================================================
// DocumentFragment Class (Boost-style)
// ============================================================================

class DocumentFragment : public Node {
public:
    typedef boost::shared_ptr<DocumentFragment> Ptr;
    
    DocumentFragment();
    virtual ~DocumentFragment() = default;
    
    static Ptr create();
};

// ============================================================================
// Node Iterator (Boost-style)
// ============================================================================

class NodeIterator {
public:
    typedef Node::Ptr NodePtr;
    
    explicit NodeIterator(const NodePtr& root);
    ~NodeIterator() = default;
    
    NodePtr next();
    NodePtr previous();
    NodePtr getCurrent() const { return current_; }
    void reset() { current_ = root_; }
    
private:
    NodePtr root_;
    NodePtr current_;
};

// ============================================================================
// Tree Walker (Boost-style)
// ============================================================================

class TreeWalker {
public:
    typedef Node::Ptr NodePtr;
    typedef std::function<bool(const NodePtr&)> FilterFunction;
    
    TreeWalker(const NodePtr& root, unsigned int whatToShow = 0xFFFFFFFF);
    ~TreeWalker() = default;
    
    NodePtr next();
    NodePtr previous();
    NodePtr parent();
    NodePtr firstChild();
    NodePtr lastChild();
    NodePtr previousSibling();
    NodePtr nextSibling();
    
    NodePtr getCurrent() const { return current_; }
    void setCurrent(const NodePtr& node) { current_ = node; }
    void setFilter(FilterFunction filter) { filter_ = filter; }
    void setWhatToShow(unsigned int whatToShow) { whatToShow_ = whatToShow; }
    
private:
    NodePtr root_;
    NodePtr current_;
    unsigned int whatToShow_;
    FilterFunction filter_;
};

} // namespace DOM
} // namespace BMS

#endif // BMS_NODE_HPP