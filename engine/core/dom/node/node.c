// node.c - DOM Node Implementation (C)
#include "node.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// ============================================================================
// Internal Helper Functions
// ============================================================================

static char* str_dup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char* result = (char*)malloc(len);
    if (result) {
        memcpy(result, str, len);
    }
    return result;
}

static void free_node_data(Node* node) {
    if (!node) return;
    
    free(node->nodeName);
    free(node->nodeValue);
    free(node->namespaceURI);
    free(node->prefix);
    free(node->localName);
    
    node->nodeName = NULL;
    node->nodeValue = NULL;
    node->namespaceURI = NULL;
    node->prefix = NULL;
    node->localName = NULL;
}

static void detach_node(Node* node) {
    if (!node) return;
    
    // Remove from parent
    if (node->parentNode) {
        Node* parent = node->parentNode;
        if (parent->firstChild == node) {
            parent->firstChild = node->nextSibling;
        }
        if (parent->lastChild == node) {
            parent->lastChild = node->previousSibling;
        }
        
        if (node->previousSibling) {
            node->previousSibling->nextSibling = node->nextSibling;
        }
        if (node->nextSibling) {
            node->nextSibling->previousSibling = node->previousSibling;
        }
        
        node->parentNode = NULL;
        node->previousSibling = NULL;
        node->nextSibling = NULL;
    }
}

static void destroy_node_recursive(Node* node, bool destroyChildren) {
    if (!node) return;
    
    if (destroyChildren) {
        Node* child = node->firstChild;
        while (child) {
            Node* next = child->nextSibling;
            destroy_node_recursive(child, true);
            child = next;
        }
    }
    
    free_node_data(node);
    free(node);
}

// ============================================================================
// Node Creation Functions
// ============================================================================

Node* node_create(NodeType type, const char* name) {
    Node* node = (Node*)calloc(1, sizeof(Node));
    if (!node) return NULL;
    
    node->type = type;
    node->nodeName = str_dup(name);
    node->nodeValue = NULL;
    node->namespaceURI = NULL;
    node->prefix = NULL;
    node->localName = NULL;
    
    node->parentNode = NULL;
    node->firstChild = NULL;
    node->lastChild = NULL;
    node->previousSibling = NULL;
    node->nextSibling = NULL;
    node->ownerDocument = NULL;
    
    // Set default function pointers
    node->destroy = (void(*)(Node*))node_destroy;
    node->cloneNode = node_clone;
    node->isEqualNode = (bool(*)(Node*, Node*))node_is_equal;
    node->appendChild = node_append_child;
    node->insertBefore = node_insert_before;
    node->replaceChild = node_replace_child;
    node->removeChild = node_remove_child;
    node->normalize = node_normalize;
    node->getTextContent = node_get_text_content;
    node->setTextContent = node_set_text_content;
    node->compareDocumentPosition = node_compare_document_position;
    node->contains = node_contains;
    node->getRootNode = node_get_root_node;
    node->render = node_render;
    
    return node;
}

Node* node_create_element(const char* tagName) {
    Node* node = node_create(NODE_TYPE_ELEMENT, tagName);
    if (node) {
        node->localName = str_dup(tagName);
    }
    return node;
}

Node* node_create_text(const char* text) {
    Node* node = node_create(NODE_TYPE_TEXT, "#text");
    if (node) {
        node->nodeValue = str_dup(text);
    }
    return node;
}

Node* node_create_comment(const char* text) {
    Node* node = node_create(NODE_TYPE_COMMENT, "#comment");
    if (node) {
        node->nodeValue = str_dup(text);
    }
    return node;
}

Node* node_create_processing_instruction(const char* target, const char* data) {
    Node* node = node_create(NODE_TYPE_PROCESSING_INSTRUCTION, target);
    if (node) {
        node->nodeValue = str_dup(data);
    }
    return node;
}

Node* node_create_document(void) {
    Node* node = node_create(NODE_TYPE_DOCUMENT, "#document");
    if (node) {
        node->ownerDocument = node;
    }
    return node;
}

Node* node_create_document_fragment(void) {
    Node* node = node_create(NODE_TYPE_DOCUMENT_FRAGMENT, "#document-fragment");
    return node;
}

void node_destroy(Node* node) {
    if (!node) return;
    destroy_node_recursive(node, true);
}

// ============================================================================
// Node Property Functions
// ============================================================================

NodeType node_get_type(Node* node) {
    return node ? node->type : -1;
}

const char* node_get_name(Node* node) {
    return node ? node->nodeName : NULL;
}

void node_set_name(Node* node, const char* name) {
    if (!node) return;
    free(node->nodeName);
    node->nodeName = str_dup(name);
}

const char* node_get_value(Node* node) {
    return node ? node->nodeValue : NULL;
}

void node_set_value(Node* node, const char* value) {
    if (!node) return;
    free(node->nodeValue);
    node->nodeValue = str_dup(value);
}

const char* node_get_namespace(Node* node) {
    return node ? node->namespaceURI : NULL;
}

void node_set_namespace(Node* node, const char* ns) {
    if (!node) return;
    free(node->namespaceURI);
    node->namespaceURI = str_dup(ns);
}

const char* node_get_prefix(Node* node) {
    return node ? node->prefix : NULL;
}

void node_set_prefix(Node* node, const char* prefix) {
    if (!node) return;
    free(node->prefix);
    node->prefix = str_dup(prefix);
}

const char* node_get_local_name(Node* node) {
    return node ? node->localName : NULL;
}

void node_set_local_name(Node* node, const char* localName) {
    if (!node) return;
    free(node->localName);
    node->localName = str_dup(localName);
}

// ============================================================================
// Node Tree Navigation
// ============================================================================

Node* node_get_parent(Node* node) {
    return node ? node->parentNode : NULL;
}

Node* node_get_first_child(Node* node) {
    return node ? node->firstChild : NULL;
}

Node* node_get_last_child(Node* node) {
    return node ? node->lastChild : NULL;
}

Node* node_get_previous_sibling(Node* node) {
    return node ? node->previousSibling : NULL;
}

Node* node_get_next_sibling(Node* node) {
    return node ? node->nextSibling : NULL;
}

Node* node_get_owner_document(Node* node) {
    if (!node) return NULL;
    
    if (node->type == NODE_TYPE_DOCUMENT) {
        return node;
    }
    
    Node* current = node;
    while (current && current->ownerDocument == NULL) {
        current = current->parentNode;
    }
    
    return current ? current->ownerDocument : NULL;
}

Node* node_get_root_node(Node* node) {
    if (!node) return NULL;
    
    Node* current = node;
    while (current->parentNode) {
        current = current->parentNode;
    }
    return current;
}

int node_get_child_count(Node* node) {
    if (!node) return 0;
    
    int count = 0;
    Node* child = node->firstChild;
    while (child) {
        count++;
        child = child->nextSibling;
    }
    return count;
}

Node* node_get_child_at(Node* node, int index) {
    if (!node || index < 0) return NULL;
    
    Node* child = node->firstChild;
    int i = 0;
    while (child) {
        if (i == index) return child;
        i++;
        child = child->nextSibling;
    }
    return NULL;
}

bool node_has_child_nodes(Node* node) {
    return node ? (node->firstChild != NULL) : false;
}

bool node_has_children(Node* node) {
    return node_has_child_nodes(node);
}

// ============================================================================
// Node Tree Manipulation
// ============================================================================

Node* node_append_child(Node* parent, Node* child) {
    if (!parent || !child) return NULL;
    
    // Don't allow appending to itself
    if (parent == child) return NULL;
    
    // Remove from current parent
    detach_node(child);
    
    // Add to parent
    child->parentNode = parent;
    
    if (parent->lastChild) {
        parent->lastChild->nextSibling = child;
        child->previousSibling = parent->lastChild;
        child->nextSibling = NULL;
        parent->lastChild = child;
    } else {
        parent->firstChild = child;
        parent->lastChild = child;
        child->previousSibling = NULL;
        child->nextSibling = NULL;
    }
    
    return child;
}

Node* node_insert_before(Node* parent, Node* newChild, Node* refChild) {
    if (!parent || !newChild) return NULL;
    
    if (refChild == NULL) {
        return node_append_child(parent, newChild);
    }
    
    // Check if refChild is a child of parent
    Node* child = parent->firstChild;
    bool found = false;
    while (child) {
        if (child == refChild) {
            found = true;
            break;
        }
        child = child->nextSibling;
    }
    
    if (!found) return NULL;
    
    // Don't allow inserting into itself
    if (parent == newChild) return NULL;
    
    // Remove from current parent
    detach_node(newChild);
    
    // Insert before refChild
    newChild->parentNode = parent;
    newChild->previousSibling = refChild->previousSibling;
    newChild->nextSibling = refChild;
    
    if (refChild->previousSibling) {
        refChild->previousSibling->nextSibling = newChild;
    } else {
        parent->firstChild = newChild;
    }
    
    refChild->previousSibling = newChild;
    
    return newChild;
}

Node* node_replace_child(Node* parent, Node* newChild, Node* oldChild) {
    if (!parent || !newChild || !oldChild) return NULL;
    
    // Check if oldChild is a child of parent
    Node* child = parent->firstChild;
    bool found = false;
    while (child) {
        if (child == oldChild) {
            found = true;
            break;
        }
        child = child->nextSibling;
    }
    
    if (!found) return NULL;
    
    // Insert new child before old child
    Node* inserted = node_insert_before(parent, newChild, oldChild);
    if (!inserted) return NULL;
    
    // Remove old child
    node_remove_child(parent, oldChild);
    
    return oldChild;
}

Node* node_remove_child(Node* parent, Node* child) {
    if (!parent || !child) return NULL;
    
    // Check if child belongs to parent
    Node* current = parent->firstChild;
    bool found = false;
    while (current) {
        if (current == child) {
            found = true;
            break;
        }
        current = current->nextSibling;
    }
    
    if (!found) return NULL;
    
    detach_node(child);
    return child;
}

void node_normalize(Node* node) {
    if (!node) return;
    
    // Merge adjacent text nodes
    Node* child = node->firstChild;
    while (child && child->nextSibling) {
        Node* next = child->nextSibling;
        
        if (child->type == NODE_TYPE_TEXT && next->type == NODE_TYPE_TEXT) {
            // Merge text nodes
            size_t len1 = child->nodeValue ? strlen(child->nodeValue) : 0;
            size_t len2 = next->nodeValue ? strlen(next->nodeValue) : 0;
            char* merged = (char*)malloc(len1 + len2 + 1);
            if (merged) {
                if (child->nodeValue) {
                    strcpy(merged, child->nodeValue);
                }
                if (next->nodeValue) {
                    strcat(merged, next->nodeValue);
                }
                free(child->nodeValue);
                child->nodeValue = merged;
                
                // Remove next node
                node_remove_child(node, next);
                node_destroy(next);
                continue;
            }
        }
        
        // Normalize children
        node_normalize(child);
        child = child->nextSibling;
    }
}

Node* node_clone(Node* node, bool deep) {
    if (!node) return NULL;
    
    Node* clone = node_create(node->type, node->nodeName);
    if (!clone) return NULL;
    
    // Copy data
    clone->nodeValue = str_dup(node->nodeValue);
    clone->namespaceURI = str_dup(node->namespaceURI);
    clone->prefix = str_dup(node->prefix);
    clone->localName = str_dup(node->localName);
    clone->userData = node->userData;
    
    // Clone children if deep
    if (deep) {
        Node* child = node->firstChild;
        while (child) {
            Node* childClone = node_clone(child, true);
            if (childClone) {
                node_append_child(clone, childClone);
            }
            child = child->nextSibling;
        }
    }
    
    return clone;
}

bool node_is_equal(Node* node, Node* other) {
    if (!node || !other) return false;
    if (node == other) return true;
    if (node->type != other->type) return false;
    if (node->nodeName && other->nodeName) {
        if (strcmp(node->nodeName, other->nodeName) != 0) return false;
    } else if (node->nodeName || other->nodeName) {
        return false;
    }
    if (node->nodeValue && other->nodeValue) {
        if (strcmp(node->nodeValue, other->nodeValue) != 0) return false;
    } else if (node->nodeValue || other->nodeValue) {
        return false;
    }
    
    // Compare children
    Node* child1 = node->firstChild;
    Node* child2 = other->firstChild;
    
    while (child1 && child2) {
        if (!node_is_equal(child1, child2)) return false;
        child1 = child1->nextSibling;
        child2 = child2->nextSibling;
    }
    
    return (child1 == NULL && child2 == NULL);
}

// ============================================================================
// Node Text Content
// ============================================================================

char* node_get_text_content(Node* node) {
    if (!node) return NULL;
    
    if (node->type == NODE_TYPE_TEXT) {
        return str_dup(node->nodeValue);
    }
    
    if (node->type == NODE_TYPE_COMMENT || node->type == NODE_TYPE_PROCESSING_INSTRUCTION) {
    if (node->type == NODE_TYPE_ELEMENT || node->type == NODE_TYPE_DOCUMENT) {
        size_t totalLen = 0;
        
        // First pass: calculate total length
        Node* child = node->firstChild;
        while (child) {
            char* text = node_get_text_content(child);
            if (text) {
                totalLen += strlen(text);
                free(text);
            }
            child = child->nextSibling;
        }
        
        // Second pass: concatenate
        char* result = (char*)malloc(totalLen + 1);
        if (!result) return NULL;
        result[0] = '\0';
        
        child = node->firstChild;
        while (child) {
            char* text = node_get_text_content(child);
            if (text) {
                strcat(result, text);
                free(text);
            }
            child = child->nextSibling;
        }
        
        return result;
    }
    
    return str_dup("");
}

void node_set_text_content(Node* node, const char* text) {
    if (!node) return;
    
    // Remove all children
    while (node->firstChild) {
        Node* child = node->firstChild;
        node_remove_child(node, child);
        node_destroy(child);
    }
    
    // Create text node
    Node* textNode = node_create_text(text ? text : "");
    if (textNode) {
        node_append_child(node, textNode);
    }
}

char* node_get_outer_html(Node* node) {
    if (!node) return NULL;
    
    if (node->type == NODE_TYPE_TEXT) {
        return str_dup(node->nodeValue);
    }

    if (node->type == NODE_TYPE_COMMENT) {
        size_t len = 7 + (node->nodeValue ? strlen(node->nodeValue) : 0);
        char* result = (char*)malloc(len + 1);
        if (!result) return NULL;
        sprintf(result, "<!--%s-->", node->nodeValue ? node->nodeValue : "");
        return result;
    }

    if (node->type == NODE_TYPE_PROCESSING_INSTRUCTION) {
        size_t len = 5 + (node->nodeName ? strlen(node->nodeName) : 0) + (node->nodeValue ? strlen(node->nodeValue) : 0);
        char* result = (char*)malloc(len + 1);
        if (!result) return NULL;
        sprintf(result, "<?%s %s?>", node->nodeName ? node->nodeName : "", node->nodeValue ? node->nodeValue : "");
        return result;
    }

        // Calculate length
        totalLen += 3; // < >
        if (node->nodeName) {
            totalLen += strlen(node->nodeName) * 2 + 2; // <tag></tag>
        }
        
        // Build opening tag
        char* result = (char*)malloc(totalLen + 1024); // generous buffer
        if (!result) return NULL;
        
        sprintf(result, "<%s", node->nodeName);
        
        // Add attributes (if any)
        // In a real implementation, attributes would be stored separately
        
        sprintf(result + strlen(result), ">");
        
        // Add children
        Node* child = node->firstChild;
        while (child) {
            char* childHTML = node_get_outer_html(child);
            if (childHTML) {
                strcat(result, childHTML);
                free(childHTML);
            }
            child = child->nextSibling;
        }
        
        // Close tag
        sprintf(result + strlen(result), "</%s>", node->nodeName);
        
        return result;
    }
    
    return str_dup("");
}

char* node_get_inner_html(Node* node) {
    if (!node) return NULL;
    
    size_t totalLen = 0;
    Node* child = node->firstChild;
    
    // Calculate total length
    while (child) {
        char* html = node_get_outer_html(child);
        if (html) {
            totalLen += strlen(html);
            free(html);
        }
        child = child->nextSibling;
    }
    
    char* result = (char*)malloc(totalLen + 1);
    if (!result) return NULL;
    result[0] = '\0';
    
    child = node->firstChild;
    while (child) {
        char* html = node_get_outer_html(child);
        if (html) {
            strcat(result, html);
            free(html);
        }
        child = child->nextSibling;
    }
    
    return result;
}

// ============================================================================
// Node Comparison
// ============================================================================

unsigned short node_compare_document_position(Node* node, Node* other) {
    if (!node || !other) return DOCUMENT_POSITION_DISCONNECTED;
    if (node == other) return 0;
    
    // Check if same root
    Node* root1 = node_get_root_node(node);
    Node* root2 = node_get_root_node(other);
    
    if (root1 != root2) {
        return DOCUMENT_POSITION_DISCONNECTED | DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC;
    }
    
    // Check if one contains the other
    if (node_contains(node, other)) {
        return DOCUMENT_POSITION_CONTAINS | DOCUMENT_POSITION_PRECEDING;
    }
    
    if (node_contains(other, node)) {
        return DOCUMENT_POSITION_CONTAINED_BY | DOCUMENT_POSITION_FOLLOWING;
    }
    
    // Check order in tree
    // This is a simplified implementation
    // In a real implementation, we would traverse the tree to determine order
    return DOCUMENT_POSITION_FOLLOWING;
}

bool node_contains(Node* node, Node* other) {
    if (!node || !other) return false;
    if (node == other) return true;
    
    Node* current = other->parentNode;
    while (current) {
        if (current == node) return true;
        current = current->parentNode;
    }
    return false;
}

bool node_is_ancestor(Node* node, Node* other) {
    return node_contains(node, other);
}

bool node_is_descendant(Node* node, Node* other) {
    return node_contains(other, node);
}

bool node_is_same(Node* node, Node* other) {
    return node == other;
}

// ============================================================================
// Node Iterator
// ============================================================================

NodeIterator* node_iterator_create(Node* root) {
    NodeIterator* iter = (NodeIterator*)calloc(1, sizeof(NodeIterator));
    if (!iter) return NULL;
    
    iter->root = root;
    iter->current = root;
    iter->filter = NULL;
    
    return iter;
}

void node_iterator_destroy(NodeIterator* iter) {
    free(iter);
}

Node* node_iterator_next(NodeIterator* iter) {
    if (!iter) return NULL;
    
    if (!iter->current) {
        iter->current = iter->root;
        return iter->current;
    }
    
    // Deep first traversal
    if (iter->current->firstChild) {
        iter->current = iter->current->firstChild;
        return iter->current;
    }
    
    if (iter->current->nextSibling) {
        iter->current = iter->current->nextSibling;
        return iter->current;
    }
    
    // Go up until we find a node with a next sibling
    Node* current = iter->current;
    while (current && current != iter->root) {
        if (current->nextSibling) {
            iter->current = current->nextSibling;
            return iter->current;
        }
        current = current->parentNode;
    }
    
    iter->current = NULL;
    return NULL;
}

Node* node_iterator_previous(NodeIterator* iter) {
    if (!iter || !iter->current) return NULL;
    
    // Go to previous sibling or parent
    if (iter->current->previousSibling) {
        iter->current = iter->current->previousSibling;
        
        // Find deepest last child
        while (iter->current->lastChild) {
            iter->current = iter->current->lastChild;
        }
        return iter->current;
    }
    
    if (iter->current->parentNode) {
        iter->current = iter->current->parentNode;
        return iter->current;
    }
    
    return NULL;
}

// ============================================================================
// Tree Walker
// ============================================================================

TreeWalker* tree_walker_create(Node* root, unsigned int whatToShow) {
    TreeWalker* walker = (TreeWalker*)calloc(1, sizeof(TreeWalker));
    if (!walker) return NULL;
    
    walker->root = root;
    walker->currentNode = root;
    walker->whatToShow = whatToShow;
    walker->acceptNode = NULL;
    
    return walker;
}

void tree_walker_destroy(TreeWalker* walker) {
    free(walker);
}

Node* tree_walker_next(TreeWalker* walker) {
    if (!walker) return NULL;
    
    Node* node = walker->currentNode;
    if (!node) return NULL;
    
    // Try first child
    if (node->firstChild && (walker->whatToShow & (1 << node->firstChild->type))) {
        walker->currentNode = node->firstChild;
        return walker->currentNode;
    }
    
    // Try next sibling
    if (node->nextSibling && (walker->whatToShow & (1 << node->nextSibling->type))) {
        walker->currentNode = node->nextSibling;
        return walker->currentNode;
    }
    
    // Go up and find next sibling
    Node* current = node;
    while (current && current != walker->root) {
        if (current->nextSibling && (walker->whatToShow & (1 << current->nextSibling->type))) {
            walker->currentNode = current->nextSibling;
            return walker->currentNode;
        }
        current = current->parentNode;
    }
    
    walker->currentNode = NULL;
    return NULL;
}

Node* tree_walker_previous(TreeWalker* walker) {
    if (!walker) return NULL;
    // Implementation similar to next but reversed
    return NULL;
}

Node* tree_walker_parent(TreeWalker* walker) {
    if (!walker || !walker->currentNode) return NULL;
    
    Node* parent = walker->currentNode->parentNode;
    if (parent && (walker->whatToShow & (1 << parent->type))) {
        walker->currentNode = parent;
        return parent;
    }
    return NULL;
}

Node* tree_walker_first_child(TreeWalker* walker) {
    if (!walker || !walker->currentNode) return NULL;
    
    Node* child = walker->currentNode->firstChild;
    while (child) {
        if (walker->whatToShow & (1 << child->type)) {
            walker->currentNode = child;
            return child;
        }
        child = child->nextSibling;
    }
    return NULL;
}

Node* tree_walker_last_child(TreeWalker* walker) {
    if (!walker || !walker->currentNode) return NULL;
    
    Node* child = walker->currentNode->lastChild;
    while (child) {
        if (walker->whatToShow & (1 << child->type)) {
            walker->currentNode = child;
            return child;
        }
        child = child->previousSibling;
    }
    return NULL;
}

Node* tree_walker_previous_sibling(TreeWalker* walker) {
    if (!walker || !walker->currentNode) return NULL;
    
    Node* sibling = walker->currentNode->previousSibling;
    while (sibling) {
        if (walker->whatToShow & (1 << sibling->type)) {
            walker->currentNode = sibling;
            return sibling;
        }
        sibling = sibling->previousSibling;
    }
    return NULL;
}

Node* tree_walker_next_sibling(TreeWalker* walker) {
    if (!walker || !walker->currentNode) return NULL;
    
    Node* sibling = walker->currentNode->nextSibling;
    while (sibling) {
        if (walker->whatToShow & (1 << sibling->type)) {
            walker->currentNode = sibling;
            return sibling;
        }
        sibling = sibling->nextSibling;
    }
    return NULL;
}

// ============================================================================
// Node Rendering
// ============================================================================

void node_render(Node* node, void* context) {
    if (!node) return;
    
    // Platform-specific rendering would go here
    printf("Render node: %s (type: %d)\n", 
           node->nodeName ? node->nodeName : "unknown", node->type);
}

void node_render_tree(Node* node, void* context, int depth) {
    if (!node) return;
    
    node_render(node, context);
    
    Node* child = node->firstChild;
    while (child) {
        node_render_tree(child, context, depth + 1);
        child = child->nextSibling;
    }
}

// ============================================================================
// Node Debugging
// ============================================================================

void node_print(Node* node, int depth) {
    if (!node) return;
    
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    
    const char* typeStr = "";
    switch (node->type) {
        case NODE_TYPE_ELEMENT: typeStr = "ELEMENT"; break;
        case NODE_TYPE_TEXT: typeStr = "TEXT"; break;
        case NODE_TYPE_COMMENT: typeStr = "COMMENT"; break;
        case NODE_TYPE_PROCESSING_INSTRUCTION: typeStr = "PROCESSING_INSTRUCTION"; break;
        case NODE_TYPE_DOCUMENT: typeStr = "DOCUMENT"; break;
        case NODE_TYPE_DOCUMENT_FRAGMENT: typeStr = "DOCUMENT_FRAGMENT"; break;
        default: typeStr = "UNKNOWN"; break;
    }
    
    printf("[%s] %s", typeStr, node->nodeName ? node->nodeName : "");
    
    if (node->nodeValue) {
        printf(" = \"%s\"", node->nodeValue);
    }
    
    printf("\n");
    
    Node* child = node->firstChild;
    while (child) {
        node_print(child, depth + 1);
        child = child->nextSibling;
    }
}

void node_print_tree(Node* node) {
    node_print(node, 0);
}

char* node_to_string(Node* node) {
    if (!node) return str_dup("null");
    
    char buffer[512];
    snprintf(buffer, sizeof(buffer), 
             "Node{type=%d, name=%s, value=%s, children=%d}",
             node->type,
             node->nodeName ? node->nodeName : "null",
             node->nodeValue ? node->nodeValue : "null",
             node_get_child_count(node));
    
    return str_dup(buffer);
}

char* node_to_xml(Node* node, bool pretty) {
    if (!node) return NULL;
    
    // This is a simplified XML serialization
    char* html = node_get_outer_html(node);
    if (!html) return NULL;
    
    // Wrap in XML declaration if pretty
    if (pretty) {
        char* result = (char*)malloc(strlen(html) + 100);
        if (result) {
            sprintf(result, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n%s", html);
            free(html);
            return result;
        }
    }
    
    return html;
}