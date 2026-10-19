// ============================================================================
// BinaryVisBase.h - helpers for a pointer-based binary tree:
//   counting nodes and freeing memory.
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

    int countNodes(const Node* n) const {
        return n == nullptr ? 0 : 1 + countNodes(n->left) + countNodes(n->right);
    }
    // Frees a whole subtree (postorder: children before the parent).
    void destroy(Node* n) {
        if (n == nullptr) return;
        destroy(n->left);
        destroy(n->right);
        delete n;
    }
};
