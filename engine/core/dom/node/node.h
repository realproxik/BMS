// node.h - DOM Node Header (C Style)
#ifndef BMS_NODE_H
#define BMS_NODE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Node Type Definitions
// ============================================================================

typedef enum {
    NODE_TYPE_ELEMENT = 1,
    NODE_TYPE_ATTRIBUTE = 2,
    NODE_TYPE_TEXT = 3,
    NODE_TYPE_CDATA_SECTION = 4,
    NODE_TYPE_PROCESSING_INSTRUCTION = 7,
    NODE_TYPE_COMMENT = 8,
    NODE_TYPE_DOCUMENT = 9,
    NODE_TYPE_DOCUMENT_TYPE = 10,
    NODE_TYPE_DOCUMENT_FRAGMENT = 11
} NodeType;

typedef enum {
    DOCUMENT_POSITION_DISCONNECTED = 0x01,
    DOCUMENT_POSITION_PRECEDING = 0x02,
    DOCUMENT_POSITION_FOLLOWING = 0x04,
    DOCUMENT_POSITION_CONTAINS = 0x08,
    DOCUMENT_POSITION_CONTAINED_BY = 0x10,
    DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC = 0x20
} DocumentPosition;

// ============================================================================
// Node Structure
// ============================================================================

typedef struct Node {
    NodeType type;
    char* nodeName;
    char* nodeValue;
    char* namespaceURI;
    char* prefix;
    char* localName;
    
    struct Node* parentNode;
    struct Node* firstChild;
    struct Node* lastChild;
    struct Node* previousSibling;
    struct Node* nextSibling;
    struct Node* ownerDocument;
    
    void* userData;
    void* extraData;
    
    // Function pointers
    void (*destroy)(struct Node* node);
    struct Node* (*cloneNode)(struct Node* node, bool deep);
    bool (*isEqualNode)(struct Node* node, struct Node* other);
    struct Node* (*appendChild)(struct Node* parent, struct Node* child);
    struct Node* (*insertBefore)(struct Node* parent, struct Node* newChild, struct Node* refChild);
    struct Node* (*replaceChild)(struct Node* parent, struct Node* newChild, struct Node* oldChild);
    struct Node* (*removeChild)(struct Node* parent, struct Node* child);
    void (*normalize)(struct Node* node);
    
    char* (*getTextContent)(struct Node* node);
    void (*setTextContent)(struct Node* node, const char* text);
    
    unsigned short (*compareDocumentPosition)(struct Node* node, struct Node* other);
    bool (*contains)(struct Node* node, struct Node* other);
    struct Node* (*getRootNode)(struct Node* node);
    
    void (*render)(struct Node* node, void* context);
} Node;

// ============================================================================
// Node Creation Functions
// ============================================================================

Node* node_create(NodeType type, const char* name);
void node_destroy(Node* node);

Node* node_create_element(const char* tagName);
Node* node_create_text(const char* text);
Node* node_create_comment(const char* text);
Node* node_create_processing_instruction(const char* target, const char* data);
Node* node_create_document(void);
Node* node_create_document_fragment(void);

// ============================================================================
// Node Property Functions
// ============================================================================

NodeType node_get_type(Node* node);
const char* node_get_name(Node* node);
void node_set_name(Node* node, const char* name);
const char* node_get_value(Node* node);
void node_set_value(Node* node, const char* value);
const char* node_get_namespace(Node* node);
void node_set_namespace(Node* node, const char* ns);
const char* node_get_prefix(Node* node);
void node_set_prefix(Node* node, const char* prefix);
const char* node_get_local_name(Node* node);
void node_set_local_name(Node* node, const char* localName);

// ============================================================================
// Node Tree Navigation
// ============================================================================

Node* node_get_parent(Node* node);
Node* node_get_first_child(Node* node);
Node* node_get_last_child(Node* node);
Node* node_get_previous_sibling(Node* node);
Node* node_get_next_sibling(Node* node);
Node* node_get_owner_document(Node* node);
Node* node_get_root_node(Node* node);

int node_get_child_count(Node* node);
Node* node_get_child_at(Node* node, int index);
bool node_has_child_nodes(Node* node);
bool node_has_children(Node* node);

// ============================================================================
// Node Tree Manipulation
// ============================================================================

Node* node_append_child(Node* parent, Node* child);
Node* node_insert_before(Node* parent, Node* newChild, Node* refChild);
Node* node_replace_child(Node* parent, Node* newChild, Node* oldChild);
Node* node_remove_child(Node* parent, Node* child);
void node_normalize(Node* node);

Node* node_clone(Node* node, bool deep);
bool node_is_equal(Node* node, Node* other);

// ============================================================================
// Node Text Content
// ============================================================================

char* node_get_text_content(Node* node);
void node_set_text_content(Node* node, const char* text);
char* node_get_outer_html(Node* node);
char* node_get_inner_html(Node* node);

// ============================================================================
// Node Comparison
// ============================================================================

unsigned short node_compare_document_position(Node* node, Node* other);
bool node_contains(Node* node, Node* other);
bool node_is_ancestor(Node* node, Node* other);
bool node_is_descendant(Node* node, Node* other);
bool node_is_same(Node* node, Node* other);

// ============================================================================
// Node Iteration
// ============================================================================

typedef struct NodeIterator {
    Node* current;
    Node* root;
    bool (*filter)(Node* node);
    void* userData;
} NodeIterator;

NodeIterator* node_iterator_create(Node* root);
void node_iterator_destroy(NodeIterator* iter);
Node* node_iterator_next(NodeIterator* iter);
Node* node_iterator_previous(NodeIterator* iter);

// ============================================================================
// Node Tree Walker
// ============================================================================

typedef struct TreeWalker {
    Node* currentNode;
    Node* root;
    unsigned int whatToShow;
    bool (*acceptNode)(struct TreeWalker* walker, Node* node);
    void* userData;
} TreeWalker;

TreeWalker* tree_walker_create(Node* root, unsigned int whatToShow);
void tree_walker_destroy(TreeWalker* walker);
Node* tree_walker_next(TreeWalker* walker);
Node* tree_walker_previous(TreeWalker* walker);
Node* tree_walker_parent(TreeWalker* walker);
Node* tree_walker_first_child(TreeWalker* walker);
Node* tree_walker_last_child(TreeWalker* walker);
Node* tree_walker_previous_sibling(TreeWalker* walker);
Node* tree_walker_next_sibling(TreeWalker* walker);

// ============================================================================
// Node Rendering
// ============================================================================

void node_render(Node* node, void* context);
void node_render_tree(Node* node, void* context, int depth);

// ============================================================================
// Node Debugging
// ============================================================================

void node_print(Node* node, int depth);
void node_print_tree(Node* node);
char* node_to_string(Node* node);
char* node_to_xml(Node* node, bool pretty);

#ifdef __cplusplus
}
#endif

#endif // BMS_NODE_H