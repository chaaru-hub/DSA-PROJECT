// ============================================================================
// BSTVis.h - Binary Search Tree: insert, delete (3 cases), search, min, max
// ============================================================================
#pragma once
#include "BinaryVisBase.h"

struct BSTNode {
    int key;
    int id;                       // unique id used by the animation system
    BSTNode* left = nullptr;
    BSTNode* right = nullptr;
};

class BSTVis : public BinaryVisBase<BSTNode> {
public:
    BSTVis();
    ~BSTVis() override { destroy(root_); }
    std::string validate() const override;

protected:
    void setup() override;
    void onAction(int id, const std::string& input) override;
    void buildScene(Frame& f) override;

private:
    enum { ActMin = ActCustom, ActMax };
    void insert(int key);
    void remove(int key);
    void search(int key);
    void findExtreme(bool findMin);
    void loadSample();
    bool contains(int key) const;
};
