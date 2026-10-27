// ============================================================================
// BinaryVisBase.h - helpers for a pointer-based binary tree:
//   counting, height, freeing memory, and the three traversal animations.
//
// 'Node' must have:  int key; int id; Node* left; Node* right;
// ============================================================================
#pragma once
#include <string>
#include "TreeVisualizer.h"


template <class Node>
class BinaryVisBase : public TreeVisualizer {
public:
    using TreeVisualizer::TreeVisualizer;

protected:
    Node* root_ = nullptr;
    int maxNodes_ = 40;            // keeps the picture readable

    bool isNil(const Node* n) const { return n == nullptr; }
    static std::string keyText(const Node* n) { return std::to_string(n->key); }

    int countNodes(const Node* n) const {
        return isNil(n) ? 0 : 1 + countNodes(n->left) + countNodes(n->right);
    }
    void destroy(Node* n) {
        if (isNil(n)) return;
        destroy(n->left);
        destroy(n->right);
        delete n;
    }

    void addTraversalButtons() {
        addButton(ActInorder, "Inorder", "Left, Node, Right (sorted order for a BST)");
        addButton(ActPreorder, "Preorder", "Node, Left, Right");
        addButton(ActPostorder, "Postorder", "Left, Right, Node");
    }

    // Returns true if 'id' was a traversal button and the animation was recorded.
    bool handleTraversal(int id) {
        if (id < ActInorder || id > ActPostorder) return false;
        static const char* names[] = {"INORDER (Left, Node, Right)", "PREORDER (Node, Left, Right)",
                                      "POSTORDER (Left, Right, Node)"};
        static const char* shortNames[] = {"Inorder", "Preorder", "Postorder"};
        int kind = id - ActInorder;
        beginOp(std::string("TRAVERSAL: ") + names[kind]);
        if (isNil(root_)) { done("The tree is empty - nothing to traverse."); return true; }
        seq_.clear();
        prefix_ = std::string(shortNames[kind]) + ":  ";
        depthFirst(root_, kind);
        done(std::string(shortNames[kind]) + " result: " + seq_);
        return true;
    }

private:
    std::string seq_, prefix_;

    void visit(Node* n, const std::string& msg) {
        seq_ += (seq_.empty() ? "" : "  ") + keyText(n);
        caption_ = prefix_ + seq_;
        setHl(n->id, Hl::Current);
        step(msg);
        setHl(n->id, Hl::Visited);
    }

    // Recursive depth-first traversal. The only difference between the three
    // orders is WHERE the "visit" happens relative to the two recursive calls.
    void depthFirst(Node* n, int kind) {
        if (isNil(n)) return;
        std::string k = keyText(n);
        if (kind == 1) visit(n, "Visit " + k + " first, then its left and right subtrees");
        depthFirst(n->left, kind);
        if (kind == 0)
            visit(n, isNil(n->left) ? k + " has no left child -> visit " + k + ", then go right"
                                    : "Left subtree of " + k + " done -> visit " + k + ", then go right");
        depthFirst(n->right, kind);
        if (kind == 2)
            visit(n, (isNil(n->left) && isNil(n->right)) ? k + " is a leaf -> visit " + k
                                                         : "Both subtrees of " + k + " done -> visit " + k);
    }
};
