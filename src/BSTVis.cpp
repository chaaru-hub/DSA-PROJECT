// ============================================================================
// BSTVis.cpp - Binary Search Tree
//
// Rule: for every node, keys in the LEFT subtree are smaller and keys in the
// RIGHT subtree are larger. Every operation walks one root-to-leaf path,
// so it costs O(h) where h is the height of the tree.
// ============================================================================
#include "BSTVis.h"
#include <climits>
#include <functional>
#include "Scene.h"

BSTVis::BSTVis() : BinaryVisBase("Binary Search Tree", "left < node < right  -  insert, delete, search") {}

void BSTVis::setup() {
    allowNumberLists();
    addButton(ActInsert, "Insert", "Insert the typed number (Enter)", true);
    addButton(ActDelete, "Delete", "Delete the typed number (Delete key)");
    addButton(ActSearch, "Search", "Search for the typed number");
    addButton(ActClear, "Clear");
    setPlaceholder("Number, e.g. 42");

    loadSample();
    showNow();
}

void BSTVis::buildScene(Frame& f) {
    layoutBinaryTree(f, root_, [this](BSTNode* n, VNode& v) {
        v.label = std::to_string(n->key);
        v.hl = hl(n->id);
    });
}

void BSTVis::onAction(int id, const std::string& input) {
    int v = 0;
    switch (id) {
    case ActInsert:
    case ActDelete:
    case ActSearch:
        if (!parseInt(input, v)) { toast("Please type a whole number first"); return; }
        if (id == ActInsert) insert(v);
        else if (id == ActDelete) remove(v);
        else search(v);
        break;
    case ActClear:
        beginOp("CLEAR");
        destroy(root_);
        root_ = nullptr;
        done("All nodes removed.");
        break;
    }
}

void BSTVis::loadSample() {
    destroy(root_);
    root_ = nullptr;
    setRecording(false);
    for (int k : {50, 30, 70, 20, 40, 60, 80, 35, 65, 85}) insert(k);
    setRecording(true);
}

// ---------------------------------------------------------------------------
// INSERT: walk down comparing, attach the new node at the empty spot found.
// ---------------------------------------------------------------------------
void BSTVis::insert(int key) {
    std::string k = std::to_string(key);
    beginOp("INSERT " + k);
    if (countNodes(root_) >= maxNodes_) { toast("Tree is full for display (40 nodes)"); return; }
    BSTNode* fresh = new BSTNode{key, newId()};
    if (!root_) {
        root_ = fresh;
        setHl(fresh->id, Hl::New);
        done("The tree is empty, so " + k + " becomes the root.");
        return;
    }
    BSTNode* cur = root_;
    while (true) {
        std::string c = std::to_string(cur->key);
        setHl(cur->id, Hl::Current);
        if (key == cur->key) {
            step(k + " = " + c + ": the key is already in the tree.");
            delete fresh;
            done("Duplicate keys are not allowed - nothing inserted.");
            return;
        }
        bool goLeft = key < cur->key;
        BSTNode*& next = goLeft ? cur->left : cur->right;
        std::string dir = goLeft ? "left" : "right";
        if (next) {
            step(k + (goLeft ? " < " : " > ") + c + ", go " + dir);
            setHl(cur->id, Hl::Visited);
            cur = next;
        } else {
            step(k + (goLeft ? " < " : " > ") + c + ", and the " + dir + " child of " + c + " is empty");
            next = fresh;
            setHl(cur->id, Hl::Visited);
            setHl(fresh->id, Hl::New);
            done("Inserted " + k + " as the " + dir + " child of " + c + ".");
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// SEARCH: same walk as insert; stop when equal or when we fall off the tree.
// ---------------------------------------------------------------------------
void BSTVis::search(int key) {
    std::string k = std::to_string(key);
    beginOp("SEARCH " + k);
    BSTNode* cur = root_;
    int depth = 0;
    while (cur) {
        std::string c = std::to_string(cur->key);
        if (key == cur->key) {
            setHl(cur->id, Hl::Found);
            done("Found " + k + " at depth " + std::to_string(depth) + " after " +
                 std::to_string(depth + 1) + " comparison(s).");
            return;
        }
        setHl(cur->id, Hl::Current);
        bool goLeft = key < cur->key;
        step(k + (goLeft ? " < " : " > ") + c + ", go " + (goLeft ? "left" : "right"));
        setHl(cur->id, Hl::Visited);
        cur = goLeft ? cur->left : cur->right;
        ++depth;
    }
    done(k + " is not in the tree (we reached an empty child).");
}

// ---------------------------------------------------------------------------
// DELETE: find the node, then one of three cases.
// ---------------------------------------------------------------------------
void BSTVis::remove(int key) {
    std::string k = std::to_string(key);
    beginOp("DELETE " + k);
    BSTNode* parent = nullptr;
    BSTNode* cur = root_;
    while (cur && cur->key != key) {
        setHl(cur->id, Hl::Current);
        bool goLeft = key < cur->key;
        step(k + (goLeft ? " < " : " > ") + std::to_string(cur->key) + ", go " + (goLeft ? "left" : "right"));
        setHl(cur->id, Hl::Visited);
        parent = cur;
        cur = goLeft ? cur->left : cur->right;
    }
    if (!cur) { done(k + " is not in the tree - nothing to delete."); return; }

    setHl(cur->id, Hl::Remove);
    // helper: make parent point to 'child' instead of 'cur'
    auto relink = [&](BSTNode* p, BSTNode* old, BSTNode* child) {
        if (!p) root_ = child;
        else if (p->left == old) p->left = child;
        else p->right = child;
    };

    if (!cur->left && !cur->right) {
        // Case 1: leaf
        step("Found " + k + ". Case 1: it is a leaf, so simply remove it.");
        relink(parent, cur, nullptr);
        delete cur;
        done("Deleted leaf " + k + ".");
    } else if (!cur->left || !cur->right) {
        // Case 2: exactly one child -> the child takes the node's place
        BSTNode* child = cur->left ? cur->left : cur->right;
        setHl(child->id, Hl::Current);
        step("Found " + k + ". Case 2: it has one child (" + std::to_string(child->key) +
             "), so link its parent directly to that child.");
        relink(parent, cur, child);
        delete cur;
        setHl(child->id, Hl::Visited);
        done("Deleted " + k + "; its subtree moved up one level.");
    } else {
        // Case 3: two children -> replace with inorder successor
        step("Found " + k + ". Case 3: two children. Find the inorder successor = smallest key in the "
             "right subtree.");
        BSTNode* sParent = cur;
        BSTNode* s = cur->right;
        setHl(s->id, Hl::Current);
        step("Go to the right child " + std::to_string(s->key) + ", then keep going left.");
        while (s->left) {
            setHl(s->id, Hl::Visited);
            sParent = s;
            s = s->left;
            setHl(s->id, Hl::Current);
            step("Go left to " + std::to_string(s->key));
        }
        setHl(s->id, Hl::Found);
        std::string sk = std::to_string(s->key);
        step("Successor is " + sk + " (it has no left child).");
        cur->key = s->key;   // copy successor's key into the node
        setHl(cur->id, Hl::New);
        step("Copy " + sk + " into the node that held " + k + ".");
        setHl(s->id, Hl::Remove);
        step("Now delete the old " + sk + " node (it has at most a right child).");
        if (sParent->left == s) sParent->left = s->right;
        else sParent->right = s->right;
        delete s;
        done("Deleted " + k + "; replaced by its successor " + sk + ".");
    }
}

// Checks the BST ordering property for every node (used by --selftest).
std::string BSTVis::validate() const {
    std::function<bool(BSTNode*, long, long)> ok = [&](BSTNode* n, long lo, long hi) {
        if (!n) return true;
        if (n->key <= lo || n->key >= hi) return false;
        return ok(n->left, lo, n->key) && ok(n->right, n->key, hi);
    };
    return ok(root_, LONG_MIN, LONG_MAX) ? "" : "BST order violated";
}
