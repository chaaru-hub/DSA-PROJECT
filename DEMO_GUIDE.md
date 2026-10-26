# Class Demo Guide – Binary Search Tree

Target length: **8–10 minutes** plus questions. Start the program before class (`make run`), press
**F11** for fullscreen, and **F2** if the projector washes out dark colours.

> Tip: before an important operation, press **Space** to pause, start the operation, then press
> **Right arrow** step by step while you talk. **Left arrow** goes back a step if someone asks
> you to repeat. **Space** lets it run again.

The banner at the bottom of the canvas always says what the algorithm is doing; read it aloud.

---

### 1. What a BST is (1 min)
* The screen opens with a sample tree. Read the **Definition** from the side panel.
* Point at any node: *"everything on its left is smaller, everything on its right is larger."*

### 2. Build a tree from the class's numbers (2 min)
* Click **Clear**. Ask a student for 6–8 numbers and type them in one go, e.g. `50 30 70 20 40 60 80`,
  then press **Enter**.
* Each insert animates in turn: *"30 < 50, go left … the left child is empty, insert here."*
* **Say:** insertion always adds a new **leaf**; the cost is the height of the tree.

### 3. Search, Min, Max (1.5 min)
* Type `40` → **Search**: *"40 < 50 go left, 40 > 30 go right, found."* Half the tree is discarded
  at every comparison, which is binary search.
* Type `45` → **Search**: the path ends at an empty child, so it is not in the tree.
* **Min** walks left until there is no left child; **Max** walks right.

### 4. Delete – the three cases (2.5 min)
Use **Load sample** first (50 30 70 20 40 60 80 35 65 85).
* **Leaf:** delete `20`. It is simply removed.
* **One child:** delete `40` (its only child is 35). The child moves up into its place.
* **Two children:** delete `50` (the root). Step through it:
  1. find the **inorder successor**: go right once, then left as far as possible (60),
  2. copy 60 into the root (watch the label cross-fade),
  3. delete the old 60 node, which has at most a right child (65 moves up).
* **Say:** the successor is the smallest key larger than the deleted one, so the BST order is kept.

### 5. Traversals (1.5 min)
* **Inorder:** the output at the top is **sorted**. *"This is why inorder of a BST = sorted order."*
* **Preorder** (root first, used to copy/save a tree), **Postorder** (children first, used to
  delete a tree).

### 6. The worst case (1 min)
* **Clear**, then type `10 20 30 40 50` and press Enter. Every node goes right, and the tree becomes
  a **linked list** of height n − 1.
* **Say:** search is O(log n) on average but **O(n) in the worst case**. Self-balancing trees
  (AVL, Red-Black) fix this by rotating.
* Point at the **Time complexity** table in the side panel.

### Closing (20 s)
* *"The algorithm records each step as a snapshot; an animation player plays those snapshots
  with easing. That is what makes pause, step and rewind work."*
* If asked about correctness: `./TreeViz --selftest` runs 5000 random operations and checks the
  BST property after each one.

---

## If something goes wrong during the demo
* Animation too slow → **Up arrow** a few times, or **End** to jump to the result.
* Tree looks cluttered → **Clear**, then **Load sample**.
* Want more room → **F1** hides the side panel.
* Typed something wrong → **Backspace**, or **Tab** to leave the input box.
