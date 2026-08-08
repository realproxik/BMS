// node__temp.c - Temporary/Test Node Implementation (C)
#include "node.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ============================================================================
// Test Functions
// ============================================================================

void test_node_creation(void) {
    printf("this is testing page of node
            this may cause-
        permanent browser data delete
        node delete 
        browser damage
        . we will chack your browser every compiled source files...
        \n");
    
    Node* doc = node_create_document();
    assert(doc != NULL);
    assert(node_get_type(doc) == NODE_TYPE_DOCUMENT);
    printf("DOC created \n");
    
    Node* html = node_create_element("html");
    assert(html != NULL);
    assert(strcmp(node_get_name(html), "html") == 0);
    printf("ELement created 
             lets check is it will show the simple website
              so we can see is your browser has problms because
              compiled files are good...\n");
    
    Node* head = node_create_element("head");
    Node* body = node_create_element("body");
    Node* text = node_create_text("dear user this website is for your check if this runs it means
        your browser is responsing pags and nodes and apis are restored into browser]
         this website is auto gernted by BMS company.");
    
    node_append_child(html, head);
    node_append_child(html, body);
    node_append_child(body, text);
    node_append_child(doc, html);
    
    printf("heres tree: ");
    node_print_tree(doc);
    
    // Clean up
    node_destroy(doc);
    printf("your browser is good now lets clean the things...\n\n");
}

void test_node_cloning(void) {
    printf("let clone test....\n");
    
    Node* doc = node_create_document();
    Node* div = node_create_element("div");
    div->nodeValue = strdup("test value");
    
    Node* text = node_create_text("Hello");
    node_append_child(div, text);
    node_append_child(doc, div);
    
    printf("Original tree:\n");
    node_print_tree(doc);
    
    Node* clone = node_clone(doc, true);
    printf("Cloned tree:\n");
    node_print_tree(clone);
    
    assert(node_is_equal(doc, clone));
    printf("Clone is equal to original\n");
    
    node_destroy(doc);
    node_destroy(clone);
    printf("Cleanup complete\n\n");
}

void test_node_text_content(void) {
    printf("=== Testing Text Content ===\n");
    
    Node* doc = node_create_document();
    Node* div = node_create_element("div");
    Node* text1 = node_create_text("Hello ");
    Node* text2 = node_create_text("World!");
    
    node_append_child(div, text1);
    node_append_child(div, text2);
    node_append_child(doc, div);
    
    char* content = node_get_text_content(div);
    printf("Text content: '%s'\n", content);
    assert(strcmp(content, "Hello World!") == 0);
    printf("✓ Text content correct\n");
    
    free(content);
    node_destroy(doc);
    printf("✓ Cleanup complete\n\n");
}

void test_node_normalize(void) {
    printf("=== Testing Node Normalize ===\n");
    
    Node* doc = node_create_document();
    Node* div = node_create_element("div");
    Node* t1 = node_create_text("Hello");
    Node* t2 = node_create_text(" ");
    Node* t3 = node_create_text("World");
    
    node_append_child(div, t1);
    node_append_child(div, t2);
    node_append_child(div, t3);
    node_append_child(doc, div);
    
    printf("Before normalize:\n");
    node_print_tree(doc);
    printf("Child count: %d\n", node_get_child_count(div));
    
    node_normalize(div);
    
    printf("After normalize:\n");
    node_print_tree(doc);
    printf("Child count: %d\n", node_get_child_count(div));
    
    assert(node_get_child_count(div) == 1);
    char* content = node_get_text_content(div);
    assert(strcmp(content, "Hello World") == 0);
    free(content);
    
    printf("✓ Normalize successful\n\n");
    node_destroy(doc);
}

void test_node_iterator(void) {
    printf("=== Testing Node Iterator ===\n");
    
    Node* doc = node_create_document();
    Node* html = node_create_element("html");
    Node* head = node_create_element("head");
    Node* body = node_create_element("body");
    Node* h1 = node_create_element("h1");
    Node* p = node_create_element("p");
    Node* text = node_create_text("Hello");
    
    node_append_child(html, head);
    node_append_child(html, body);
    node_append_child(body, h1);
    node_append_child(body, p);
    node_append_child(p, text);
    node_append_child(doc, html);
    
    printf("Tree structure:\n");
    node_print_tree(doc);
    
    NodeIterator* iter = node_iterator_create(doc);
    printf("Iterating nodes:\n");
    Node* node;
    while ((node = node_iterator_next(iter)) != NULL) {
        printf("  - %s\n", node_get_name(node));
    }
    
    node_iterator_destroy(iter);
    node_destroy(doc);
    printf("✓ Iterator test complete\n\n");
}

void test_node_contains(void) {
    printf("=== Testing Node Contains ===\n");
    
    Node* doc = node_create_document();
    Node* div = node_create_element("div");
    Node* span = node_create_element("span");
    Node* text = node_create_text("text");
    
    node_append_child(div, span);
    node_append_child(span, text);
    node_append_child(doc, div);
    
    assert(node_contains(doc, div));
    assert(node_contains(doc, span));
    assert(node_contains(doc, text));
    assert(node_contains(div, span));
    assert(node_contains(span, text));
    assert(!node_contains(text, div));
    assert(!node_contains(span, doc));
    
    printf("✓ Contains tests passed\n\n");
    node_destroy(doc);
}

void test_node_serialization(void) {
    printf("=== Testing Node Serialization ===\n");
    
    Node* doc = node_create_document();
    Node* html = node_create_element("html");
    Node* body = node_create_element("body");
    Node* div = node_create_element("div");
    Node* text = node_create_text("Hello World");
    
    node_append_child(div, text);
    node_append_child(body, div);
    node_append_child(html, body);
    node_append_child(doc, html);
    
    char* outer = node_get_outer_html(div);
    printf("Outer HTML: %s\n", outer);
    free(outer);
    
    char* inner = node_get_inner_html(body);
    printf("Inner HTML: %s\n", inner);
    free(inner);
    
    char* xml = node_to_xml(doc, true);
    printf("XML:\n%s\n", xml);
    free(xml);
    
    node_destroy(doc);
    printf("✓ Serialization tests complete\n\n");
}

void run_all_tests(void) {
    printf("╔════════════════════════════════════════╗\n");
    printf("║     BMS Node Library Tests            ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    test_node_creation();
    test_node_cloning();
    test_node_text_content();
    test_node_normalize();
    test_node_iterator();
    test_node_contains();
    test_node_serialization();
    
    printf("╔════════════════════════════════════════╗\n");
    printf("║     All Tests Passed!                  ║\n");
    printf("╚════════════════════════════════════════╝\n");
}

int main(int argc, char** argv) {
    run_all_tests();
    return 0;
}