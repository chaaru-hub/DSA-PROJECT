// ============================================================================
// Scene.h - draws (and blends) animation frames, plus the tree layout helper
// ============================================================================
#pragma once
#include <SDL.h>
#include <functional>
#include <vector>
#include "Anim.h"
#include "Gfx.h"

// Draw the blend of frames A -> B at progress t (0..1) inside 'area'.
void drawScene(Gfx& g, const Frame& A, const Frame& B, float t, const SDL_FRect& area, float time);

// ----------------------------------------------------------------------------
// Binary tree layout: x = in-order index, y = depth.
// In-order placement guarantees that a left child is always left of its
// parent and a right child always right, which is exactly the BST picture
// (reading the nodes from left to right gives the keys in sorted order).
// ----------------------------------------------------------------------------
template <class Node, class Decorate>
void layoutBinaryTree(Frame& f, Node* root, Decorate decorate) {
    int column = 0;
    std::function<void(Node*, int, int)> rec = [&](Node* n, int depth, int parentId) {
        if (n == nullptr) return;
        rec(n->left, depth + 1, n->id);
        VNode v;
        v.id = n->id;
        v.x = (float)column++;
        v.y = (float)depth;
        decorate(n, v);
        f.nodes.push_back(v);
        if (parentId >= 0) f.edges.push_back(VEdge{parentId, n->id});
        rec(n->right, depth + 1, n->id);
    };
    rec(root, 0, -1);
}
