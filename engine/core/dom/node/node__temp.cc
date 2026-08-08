// node__temp.cc - Temporary/Test Node Implementation (C++)
#include "node.hh"
#include <iostream>
#include <memory>
#include <cassert>

using namespace BMS::DOM;

// ============================================================================
// Test Functions
// ============================================================================

void test_node_creation() {
    std::cout << "=== Testing Node Creation ===" << std::endl;
    
    auto doc = Node::createDocument();
    assert(doc != nullptr);
    assert(doc->getType() == NodeType::DOCUMENT);
    std::cout << "✓ Document created" << std::endl;
    
    auto html = Node::createElement("html");
    assert(html != nullptr);
    assert(html->getNodeName() == "html");
    std::cout << "✓ Element created" << std::endl;
    
    auto head = Node::createElement("head");
    auto body = Node::createElement("body");
    auto text = Node::createTextNode("Hello World");
    
    html->appendChild(std::move(head));
    html->appendChild(std::move(body));
    body->appendChild(std::move(text));
    doc->appendChild(std::move(html));
    
    std::cout << "✓ Tree built:" << std::endl;
    doc->printTree();
    std::cout << std::endl;
}

void test_node_cloning() {
    std::cout << "=== Testing Node Cloning ===" << std::endl;
    
    auto doc = Node::createDocument();
    auto div = Node::createElement("div");
    div->setNodeValue("test value");
    
    auto text = Node::createTextNode("Hello");
    div->appendChild(std::move(text));
    doc->appendChild(std::move(div));
    
    std::cout << "Original tree:" << std::endl;
    doc->printTree();
    
    auto clone = doc->clone(true);
    std::cout << "Cloned tree:" << std::endl;
    clone->printTree();
    
    assert(doc->isEqual(clone.get()));
    std::cout << "✓ Clone is equal to original" << std::endl << std::endl;
}

void test_node_text_content() {
    std::cout << "=== Testing Text Content ===" << std::endl;
    
    auto doc = Node::createDocument();
    auto div = Node::createElement("div");
    auto text1 = Node::createTextNode("Hello ");
    auto text2 = Node::createTextNode("World!");
    
    div->appendChild(std::move(text1));
    div->appendChild(std::move(text2));
    doc->appendChild(std::move(div));
    
    std::string content = doc->getFirstChild()->getTextContent();
    std::cout << "Text content: '" << content << "'" << std::endl;
    assert(content == "Hello World!");
    std::cout << "✓ Text content correct" << std::endl << std::endl;
}

void test_node_normalize() {
    std::cout << "=== Testing Node Normalize ===" << std::endl;
    
    auto doc = Node::createDocument();
    auto div = Node::createElement("div");
    auto t1 = Node::createTextNode("Hello");
    auto t2 = Node::createTextNode(" ");
    auto t3 = Node::createTextNode("World");
    
    div->appendChild(std::move(t1));
    div->appendChild(std::move(t2));
    div->appendChild(std::move(t3));
    doc->appendChild(std::move(div));
    
    std::cout << "Before normalize:" << std::endl;
    doc->printTree();
    std::cout << "Child count: " << doc->getFirstChild()->getChildCount() << std::endl;
    
    doc->getFirstChild()->normalize();
    
    std::cout << "After normalize:" << std::endl;
    doc->printTree();
    std::cout << "Child count: " << doc->getFirstChild()->getChildCount() << std::endl;
    
    assert(doc->getFirstChild()->getChildCount() == 1);
    std::string content = doc->getFirstChild()->getTextContent();
    assert(content == "Hello World");
    
    std::cout << "✓ Normalize successful" << std::endl << std::endl;
}

void test_node_iterator() {
    std::cout << "=== Testing Node Iterator ===" << std::endl;
    
    auto doc = Node::createDocument();
    auto html = Node::createElement("html");
    auto head = Node::createElement("head");
    auto body = Node::createElement("body");
    auto h1 = Node::createElement("h1");
    auto p = Node::createElement("p");
    auto text = Node::createTextNode("Hello");
    
    html->appendChild(std::move(head));
    html->appendChild(std::move(body));
    body->appendChild(std::move(h1));
    body->appendChild(std::move(p));
    p->appendChild(std::move(text));
    doc->appendChild(std::move(html));
    
    std::cout << "Tree structure:" << std::endl;
    doc->printTree();
    
    NodeIterator iter(doc.get());
    std::cout << "Iterating nodes:" << std::endl;
    Node* node;
    while ((node = iter.next()) != nullptr) {
        std::cout << "  - " << node->getNodeName() << std::endl;
    }
    
    std::cout << "✓ Iterator test complete" << std::endl << std::endl;
}

void test_node_contains() {
    std::cout << "=== Testing Node Contains ===" << std::endl;
    
    auto doc = Node::createDocument();
    auto div = Node::createElement("div");
    auto span = Node::createElement("span");
    auto text = Node::createTextNode("text");
    
    span->appendChild(std::move(text));
    div->appendChild(std::move(span));
    doc->appendChild(std::move(div));
    
    auto d = doc->getFirstChild();
    auto s = d->getFirstChild();
    auto t = s->getFirstChild();
    
    assert(doc->contains(d.get()));
    assert(doc->contains(s.get()));
    assert(doc->contains(t.get()));
    assert(d->contains(s.get()));
    assert(s->contains(t.get()));
    assert(!t->contains(d.get()));
    assert(!s->contains(d.get()));
    
    std::cout << "✓ Contains tests passed" << std::endl << std::endl;
}

void test_node_serialization() {
    std::cout << "=== Testing Node Serialization ===" << std::endl;
    
    auto doc = Node::createDocument();
    auto html = Node::createElement("html");
    auto body = Node::createElement("body");
    auto pi = Node::createProcessingInstruction("xml-stylesheet", "type='text/xsl' href='style.xsl'");
    auto comment = Node::createComment("This is a comment");
    auto div = Node::createElement("div");
    auto text = Node::createTextNode("Hello World");
    
    div->appendChild(std::move(text));
    body->appendChild(std::move(pi));
    body->appendChild(std::move(comment));
    body->appendChild(std::move(div));
    html->appendChild(std::move(body));
    doc->appendChild(std::move(html));
    
    std::string outer = doc->getFirstChild()->getFirstChild()->getFirstChild()->getOuterHTML();
    std::cout << "Outer HTML: " << outer << std::endl;
    
    std::string inner = doc->getFirstChild()->getFirstChild()->getInnerHTML();
    std::cout << "Inner HTML: " << inner << std::endl;
    
    std::string xml = doc->toXML(true);
    std::cout << "XML:\n" << xml << std::endl;
    
    std::cout << "✓ Serialization tests complete" << std::endl << std::endl;
}

void test_node_special_nodes() {
    std::cout << "=== Testing Special Nodes ===" << std::endl;
    
    auto doc = Node::createDocument();
    auto comment = Node::createComment("Hello comment");
    auto pi = Node::createProcessingInstruction("xml-stylesheet", "type='text/css' href='style.css'");
    auto text = Node::createTextNode("Sample text");
    
    doc->appendChild(std::move(comment));
    doc->appendChild(std::move(pi));
    doc->appendChild(std::move(text));
    
    doc->printTree();
    
    assert(doc->getChildAt(0)->getType() == NodeType::COMMENT);
    assert(doc->getChildAt(1)->getType() == NodeType::PROCESSING_INSTRUCTION);
    assert(doc->getChildAt(2)->getType() == NodeType::TEXT);
    
    std::cout << "Comment outer HTML: " << doc->getChildAt(0)->getOuterHTML() << std::endl;
    std::cout << "PI outer HTML: " << doc->getChildAt(1)->getOuterHTML() << std::endl;
    std::cout << "Text outer HTML: " << doc->getChildAt(2)->getOuterHTML() << std::endl;
    
    std::cout << "✓ Special node tests complete" << std::endl << std::endl;
}

void run_all_tests() {
    std::cout << "╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║     BMS Node C++ Library Tests        ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl << std::endl;
    
    test_node_creation();
    test_node_cloning();
    test_node_text_content();
    test_node_normalize();
    test_node_iterator();
    test_node_contains();
    test_node_serialization();
    
    std::cout << "╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║     All Tests Passed! ✓               ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;
}

int main(int argc, char** argv) {
    run_all_tests();
    return 0;
}