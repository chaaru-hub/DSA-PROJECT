# Viva Questions and Answers – Binary Search Tree (25)

### 1. What is a binary search tree?
A binary tree in which, for every node, all keys in the left subtree are smaller than the
node's key and all keys in the right subtree are larger. Each comparison tells us which half of
the tree can be ignored, as in binary search on a sorted array.

### 2. What is the difference between a binary tree and a BST?
A binary tree only limits each node to at most two children; keys can be anywhere. A BST adds the
ordering rule (left < node < right), which makes search O(h) instead of O(n).

### 3. How does search work, and what does it cost?
Start at the root. If the key equals the node, it is found; if smaller, go left; if larger, go
right. If we reach an empty child, the key is not in the tree. The cost is O(h), where h is the
height: O(log n) for a balanced tree, O(n) for a skewed one.

### 4. How does insertion work?
Search for the key as above. When the search reaches an empty child pointer, create the new node
there. A new key always becomes a **leaf**. Cost O(h).

### 5. Why does an inorder traversal of a BST give sorted output?
Inorder visits the left subtree, then the node, then the right subtree. By the BST rule all keys
on the left are smaller and all on the right are larger, so (by induction on the subtrees) the
keys come out in increasing order.

### 6. Explain the three cases of deletion.
1. **Leaf:** remove it (set the parent's pointer to null).
2. **One child:** link the parent directly to that child.
3. **Two children:** find the inorder successor (the smallest key in the right subtree), copy its
   key into the node, then delete the successor node, which has at most a right child, so it is
   case 1 or 2.

### 7. Why can the inorder successor never have a left child?
It is found by going right once and then left as far as possible. If it had a left child we
would have gone further left, so the node we stop at has no left child.

### 8. Can we use the inorder predecessor instead of the successor?
Yes. The predecessor (the largest key in the left subtree) works the same way: it has no right
child. Both keep the BST property; some books use one, some the other. This program uses the
successor, as CLRS does.

### 9. Where are the minimum and maximum in a BST?
The minimum is the leftmost node (keep going left); the maximum is the rightmost node (keep going
right). Cost O(h).

### 10. What is the best, average and worst-case time for search / insert / delete?
Best O(1) (the key is at the root). Average O(log n) for random insertion order. Worst O(n) when
the tree is skewed (for example, keys inserted in sorted order).

### 11. When does a BST become a "linked list"? Show an example.
Inserting already sorted data: 10, 20, 30, 40, 50. Each key is larger than all the previous ones,
so it always goes right; the height becomes n − 1 and every operation is O(n).

### 12. How can we avoid the worst case?
Use a **self-balancing** BST, such as an AVL tree (heights of the subtrees differ by at most 1) or
a Red-Black tree (colour rules). They rotate after insert/delete to keep the height O(log n).
Alternatively, shuffle the keys before inserting.

### 13. What are the traversals of a binary tree and their uses?
* **Inorder** (L, N, R): sorted output of a BST.
* **Preorder** (N, L, R): copying or saving a tree; recreating the same BST by re-inserting.
* **Postorder** (L, R, N): deleting / freeing a tree (children before parent).
* **Level order** (breadth-first, with a queue): printing level by level, shortest-path style problems.

### 14. How is level-order traversal implemented?
With a queue: enqueue the root; repeatedly dequeue a node, visit it, and enqueue its left and right
children. In this project the queue is our own circular-array `Queue` class.

### 15. What is the space complexity of a BST and of its operations?
The tree uses O(n) memory (one node per key with two pointers). Iterative search/insert use O(1)
extra space; recursive versions and traversals use O(h) stack space; level order uses O(w) queue
space, where w is the maximum width.

### 16. What are the height and depth of a node? What is the height of a BST with n nodes?
Depth = number of edges from the root to the node; height = number of edges on the longest path
from the node down to a leaf. A BST with n nodes has height between ⌊log₂ n⌋ (complete) and
n − 1 (skewed).

### 17. How many different BSTs can be formed with n distinct keys?
The n-th **Catalan number**, C(n) = (2n)! / ((n+1)! n!). For example, 3 keys give 5 different BSTs.

### 18. Does the insertion order matter?
Yes. The same set of keys gives different shapes for different orders: 50, 30, 70 gives a
balanced tree, while 30, 50, 70 gives a right-leaning chain. The inorder output is the same for
all of them (sorted).

### 19. Can a BST contain duplicate keys?
The standard definition uses distinct keys (this program rejects duplicates). If duplicates are
needed, either store a count in each node or consistently put equal keys on one side
(e.g. left ≤ node < right).

### 20. How do you check whether a binary tree is a valid BST?
Pass a range (min, max) down the recursion: the root may be anything, a left child must be in
(min, parent) and a right child in (parent, max). Checking only parent and child is **not**
enough. Alternatively, an inorder traversal must be strictly increasing. (This program's
`--selftest` uses the range method.)

### 21. How do you find the inorder successor of a node in general?
If the node has a right subtree, the successor is the minimum of that subtree. Otherwise it is
the nearest ancestor whose left subtree contains the node (walk down from the root, remembering
the last node where you went left).

### 22. BST vs sorted array vs hash table?
* **Sorted array:** search O(log n), but insert/delete O(n) (shifting elements).
* **BST:** search, insert and delete all O(h), with sorted order and range queries for free.
* **Hash table:** O(1) average search/insert, but no ordering, no min/max, no range queries.

### 23. What are the real-world uses of BSTs?
Ordered maps and sets (`std::map`, Java `TreeMap`, which use balanced BSTs), symbol tables in
compilers, database and file-system indexes (via B-trees, a generalisation), range queries
("all students with marks between 60 and 80"), and tree sort.

### 24. What is tree sort and what is its complexity?
Insert all keys into a BST, then do an inorder traversal. O(n log n) on average, O(n²) in the
worst case (sorted input) unless a balanced BST is used.

### 25. How does your program animate the algorithm? (implementation question)
The algorithm runs once on the real tree and calls `step(message)` at every comparison or
change. `step()` records a **snapshot** (Frame) of all nodes (id, position, label, colour), the
edges and the log text into a **queue**. The `AnimPlayer` plays that queue and interpolates
between consecutive frames with ease-in-out easing: nodes are matched by id, so moved nodes
glide, new nodes grow in, deleted nodes fade out, and edges follow the animated positions.
Because every step is stored, pause, single-step, step-back and speed control are simple. Nodes
are placed by inorder index (x) and depth (y), so the picture always shows the BST order from
left to right. Shapes are drawn with `SDL_RenderGeometry`, and a 1-pixel soft edge gives
anti-aliasing. The program runs at 60 FPS with VSync.

---

### Bonus quick-fire answers
* **Recursive vs iterative search?** Same O(h) time; iterative uses O(1) space, recursive O(h) stack.
* **Which traversal frees a tree safely?** Postorder (children are freed before the parent).
* **Which traversal lets you rebuild the same BST by inserting?** Preorder (or level order).
* **Inorder + preorder → unique tree?** Yes, for distinct keys, any binary tree can be rebuilt
  from its inorder and preorder sequences.
