/*
 * test_rbtree.c — comprehensive test suite for the red-black tree.
 *
 * NOTE: rbtree_insert() uses `value` for BST ordering (traversal compares
 * node->value), while rbtree_search() / rbtree_remove() use `key`.
 * Therefore every test below keeps key == value so that both operations
 * traverse the same path and results are predictable.
 *
 * Insert cases covered
 * --------------------
 *  I-1  Insert into empty tree          → root must be black
 *  I-2  Parent is black                 → no rotation, new node stays red
 *  I-3  Uncle is red                    → recolor (push red up)
 *  I-4  Left-Left  (uncle black, LL)    → single right rotation
 *  I-5  Left-Right (uncle black, LR)    → left-rotate parent + right-rotate GP
 *  I-6  Right-Right(uncle black, RR)    → single left rotation
 *  I-7  Right-Left (uncle black, RL)    → right-rotate parent + left-rotate GP
 *  I-8  Cascading recolors              → recolor propagates up multiple levels
 *
 * Delete cases covered
 * --------------------
 *  D-1  Delete non-existent key         → tree unchanged
 *  D-2  Delete the only node            → tree becomes NULL
 *  D-3  Delete a red leaf               → simple removal, no fix needed
 *  D-4  Delete a black node, one red child → transplant + recolor child black
 *  D-5  Delete node with two children   → replaced by in-order successor
 *  D-6  Fix-up: sibling is red          → rotate + recolor, resolve new sibling
 *  D-7  Fix-up: sibling black, both nephews black, parent black → propagate up
 *  D-8  Fix-up: sibling black, both nephews black, parent red  → stop early
 *  D-9  Fix-up: sibling black, right nephew red (left nephew any) → single rotate
 *  D-10 Fix-up: sibling black, only left nephew red             → double rotate
 *  D-11 Delete all nodes one by one     → tree ends as NULL
 *
 * Invariants verified after every operation
 * -----------------------------------------
 *  • Root is black (or NULL for empty tree)
 *  • No two consecutive red nodes (no red-red violation)
 *  • Equal black-height on every root-to-leaf path
 *  • BST ordering: left subtree values < node value ≤ right subtree values
 *  • Parent pointers are consistent
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include "rbtree.h"

/* ═══════════════════════════════════════════════════════════════
 * Mini test framework
 * ═══════════════════════════════════════════════════════════════ */
static int g_passed = 0;
static int g_failed = 0;

#define ASSERT(cond, msg) \
    do { \
        if (cond) { \
            printf("  [PASS] %s\n", msg); \
            g_passed++; \
        } else { \
            printf("  [FAIL] %s  (line %d)\n", msg, __LINE__); \
            g_failed++; \
        } \
    } while (0)

#define SECTION(name) \
    printf("\n─── %s ───\n", name)

/* ═══════════════════════════════════════════════════════════════
 * RB-tree invariant helpers
 * ═══════════════════════════════════════════════════════════════ */

/* Returns black-height of the subtree rooted at `node`, or -1 on violation. */
static int black_height(const RBTree *node)
{
    if (node == NULL) return 0;

    int lh = black_height(node->left);
    int rh = black_height(node->right);

    if (lh < 0 || rh < 0) return -1;          /* propagate error */
    if (lh != rh)          return -1;          /* unequal black heights */

    return lh + (node->is_black ? 1 : 0);
}

/* Returns false if any red-red violation is found. */
static bool no_red_red(const RBTree *node)
{
    if (node == NULL) return true;
    if (!node->is_black) {
        if (node->left  && !node->left->is_black)  return false;
        if (node->right && !node->right->is_black) return false;
    }
    return no_red_red(node->left) && no_red_red(node->right);
}

/*
 * BST ordering check: every node's value must be in (min_val, max_val].
 * Insert goes LEFT when value < current->value, RIGHT otherwise (≥).
 * So a left child has value strictly less than its parent,
 * and a right child has value ≥ its parent.
 */
static bool bst_valid(const RBTree *node, int min_excl, int max_incl)
{
    if (node == NULL) return true;
    /* node->value must satisfy: min_excl < value <= max_incl */
    if (node->value <= min_excl) return false;
    if (node->value > max_incl)  return false;
    return bst_valid(node->left,  min_excl,      node->value - 1) &&
           bst_valid(node->right, node->value - 1, max_incl);
}

/* Parent-pointer consistency check. */
static bool parents_ok(const RBTree *node, const RBTree *expected_parent)
{
    if (node == NULL) return true;
    if (node->parent != expected_parent) return false;
    return parents_ok(node->left,  node) &&
           parents_ok(node->right, node);
}

/* Count nodes. */
static int count_nodes(const RBTree *node)
{
    if (node == NULL) return 0;
    return 1 + count_nodes(node->left) + count_nodes(node->right);
}

/* Master validity check — runs all four invariants. */
static bool is_valid_rbtree(const RBTree *root)
{
    if (root == NULL) return true;                   /* empty tree is valid */
    if (!root->is_black)           return false;     /* root must be black  */
    if (!no_red_red(root))         return false;     /* no red-red          */
    if (black_height(root) < 0)    return false;     /* equal black-heights */
    if (!bst_valid(root, INT_MIN, INT_MAX)) return false; /* BST property   */
    if (!parents_ok(root, NULL))   return false;     /* parent pointers     */
    return true;
}

/* ═══════════════════════════════════════════════════════════════
 * Helper: free the entire tree
 * ═══════════════════════════════════════════════════════════════ */
static void free_tree(RBTree *node)
{
    if (node == NULL) return;
    free_tree(node->left);
    free_tree(node->right);
    free(node);
}

/* ═══════════════════════════════════════════════════════════════
 * INSERT TESTS
 * ═══════════════════════════════════════════════════════════════ */

/* I-1: Insert into empty tree → root must be black */
static void test_insert_empty(void)
{
    SECTION("I-1: Insert into empty tree");
    RBTree *root = NULL;
    root = rbtree_insert(root, 10, 10);

    ASSERT(root != NULL,           "root is not NULL after first insert");
    ASSERT(root->is_black,         "root is black");
    ASSERT(root->key   == 10,      "root key == 10");
    ASSERT(root->value == 10,      "root value == 10");
    ASSERT(root->left  == NULL,    "root has no left child");
    ASSERT(root->right == NULL,    "root has no right child");
    ASSERT(root->parent == NULL,   "root parent is NULL");
    ASSERT(is_valid_rbtree(root),  "tree is a valid RB tree");

    free_tree(root);
}

/* I-2: Insert when parent is black → new node stays red, no rotation */
static void test_insert_parent_black(void)
{
    SECTION("I-2: Parent is black (no fix needed)");
    RBTree *root = NULL;
    root = rbtree_insert(root, 10, 10);
    root = rbtree_insert(root,  5,  5);
    root = rbtree_insert(root, 15, 15);

    ASSERT(root->key == 10,                   "root is still 10");
    ASSERT(root->is_black,                    "root is black");
    ASSERT(root->left  && !root->left->is_black,  "left child (5) is red");
    ASSERT(root->right && !root->right->is_black, "right child (15) is red");
    ASSERT(count_nodes(root) == 3,            "tree has 3 nodes");
    ASSERT(is_valid_rbtree(root),             "tree is a valid RB tree");

    free_tree(root);
}

/* I-3: Uncle is red → recolor (push red up, root stays black) */
static void test_insert_uncle_red(void)
{
    SECTION("I-3: Uncle is red → recolor");
    /* Tree after 10,5,15 → 10(B), 5(R), 15(R)
       Insert 3 → parent=5(R) uncle=15(R) → recolor: 5,15→B, 10→R then →B */
    RBTree *root = NULL;
    root = rbtree_insert(root, 10, 10);
    root = rbtree_insert(root,  5,  5);
    root = rbtree_insert(root, 15, 15);
    root = rbtree_insert(root,  3,  3);

    ASSERT(root->is_black,                    "root is black after recolor");
    ASSERT(root->key == 10,                   "root key is 10");
    ASSERT(root->left->is_black,              "5 became black");
    ASSERT(root->right->is_black,             "15 became black");
    ASSERT(!root->left->left->is_black,       "3 is red");
    ASSERT(count_nodes(root) == 4,            "tree has 4 nodes");
    ASSERT(is_valid_rbtree(root),             "tree is a valid RB tree");

    free_tree(root);
}

/* I-4: Left-Left case → single right rotation */
static void test_insert_ll(void)
{
    SECTION("I-4: Left-Left → single right rotation");
    /* Insert 30, 20, 10: 10 is left child of 20 which is left child of 30.
       Parent=20(R), uncle=NULL(B), LL → right-rotate(30), swap colors.
       Result: 20(B) as root, 10(R) left, 30(R) right. */
    RBTree *root = NULL;
    root = rbtree_insert(root, 30, 30);
    root = rbtree_insert(root, 20, 20);
    root = rbtree_insert(root, 10, 10);

    ASSERT(root->key == 20,         "20 becomes root");
    ASSERT(root->is_black,          "new root is black");
    ASSERT(root->left->key  == 10,  "10 is left child");
    ASSERT(root->right->key == 30,  "30 is right child");
    ASSERT(!root->left->is_black,   "10 is red");
    ASSERT(!root->right->is_black,  "30 is red");
    ASSERT(is_valid_rbtree(root),   "tree is a valid RB tree");

    free_tree(root);
}

/* I-5: Left-Right case → left-rotate parent + right-rotate grandparent */
static void test_insert_lr(void)
{
    SECTION("I-5: Left-Right → double rotation (left then right)");
    /* Insert 30, 10, 20: 20 is right child of 10 which is left child of 30.
       Left-rotate(10) turns it into LL, then right-rotate(30).
       Result: 20(B) as root, 10(R) left, 30(R) right. */
    RBTree *root = NULL;
    root = rbtree_insert(root, 30, 30);
    root = rbtree_insert(root, 10, 10);
    root = rbtree_insert(root, 20, 20);

    ASSERT(root->key == 20,         "20 becomes root");
    ASSERT(root->is_black,          "new root is black");
    ASSERT(root->left->key  == 10,  "10 is left child");
    ASSERT(root->right->key == 30,  "30 is right child");
    ASSERT(!root->left->is_black,   "10 is red");
    ASSERT(!root->right->is_black,  "30 is red");
    ASSERT(is_valid_rbtree(root),   "tree is a valid RB tree");

    free_tree(root);
}

/* I-6: Right-Right case → single left rotation */
static void test_insert_rr(void)
{
    SECTION("I-6: Right-Right → single left rotation");
    /* Insert 10, 20, 30: 30 is right child of 20 which is right child of 10.
       Left-rotate(10). Result: 20(B), 10(R) left, 30(R) right. */
    RBTree *root = NULL;
    root = rbtree_insert(root, 10, 10);
    root = rbtree_insert(root, 20, 20);
    root = rbtree_insert(root, 30, 30);

    ASSERT(root->key == 20,         "20 becomes root");
    ASSERT(root->is_black,          "new root is black");
    ASSERT(root->left->key  == 10,  "10 is left child");
    ASSERT(root->right->key == 30,  "30 is right child");
    ASSERT(!root->left->is_black,   "10 is red");
    ASSERT(!root->right->is_black,  "30 is red");
    ASSERT(is_valid_rbtree(root),   "tree is a valid RB tree");

    free_tree(root);
}

/* I-7: Right-Left case → right-rotate parent + left-rotate grandparent */
static void test_insert_rl(void)
{
    SECTION("I-7: Right-Left → double rotation (right then left)");
    /* Insert 10, 30, 20: 20 is left child of 30 which is right child of 10.
       Right-rotate(30) turns it into RR, then left-rotate(10).
       Result: 20(B), 10(R) left, 30(R) right. */
    RBTree *root = NULL;
    root = rbtree_insert(root, 10, 10);
    root = rbtree_insert(root, 30, 30);
    root = rbtree_insert(root, 20, 20);

    ASSERT(root->key == 20,         "20 becomes root");
    ASSERT(root->is_black,          "new root is black");
    ASSERT(root->left->key  == 10,  "10 is left child");
    ASSERT(root->right->key == 30,  "30 is right child");
    ASSERT(!root->left->is_black,   "10 is red");
    ASSERT(!root->right->is_black,  "30 is red");
    ASSERT(is_valid_rbtree(root),   "tree is a valid RB tree");

    free_tree(root);
}

/* I-8: Cascading recolors → red propagates up multiple levels */
static void test_insert_cascade_recolor(void)
{
    SECTION("I-8: Cascading recolors");
    /*
     * Build: 10, 5, 15, 3, 7, 12, 17, then insert 1.
     * After 10,5,15 → 10(B),5(R),15(R).
     * Insert 3 → uncle=15(R) → recolor: 5,15→B, root stays B. Result:
     *   10(B), 5(B), 15(B), 3(R).
     * Insert 7 → parent=5(B) → no fix: 7(R) under 5(B). 
     * Insert 12 → parent=15(B) → no fix: 12(R).
     * Insert 17 → parent=15(B) → no fix: 17(R).
     * Insert 1 → parent=3(R), uncle=7(R) → recolor: 3,7→B, 5→R.
     *   5's parent is 10(B) → stop. Root still 10(B).
     */
    RBTree *root = NULL;
    root = rbtree_insert(root, 10, 10);
    root = rbtree_insert(root,  5,  5);
    root = rbtree_insert(root, 15, 15);
    root = rbtree_insert(root,  3,  3);
    root = rbtree_insert(root,  7,  7);
    root = rbtree_insert(root, 12, 12);
    root = rbtree_insert(root, 17, 17);
    root = rbtree_insert(root,  1,  1);

    ASSERT(root->key == 10,                        "root is 10");
    ASSERT(root->is_black,                         "root is black");
    ASSERT(count_nodes(root) == 8,                 "8 nodes in tree");
    ASSERT(rbtree_search(root, 1)  != NULL,        "key 1 found");
    ASSERT(rbtree_search(root, 17) != NULL,        "key 17 found");
    ASSERT(is_valid_rbtree(root),                  "tree is a valid RB tree");

    free_tree(root);
}

/* ═══════════════════════════════════════════════════════════════
 * DELETE TESTS
 * ═══════════════════════════════════════════════════════════════ */

/* D-1: Delete non-existent key → tree unchanged */
static void test_delete_nonexistent(void)
{
    SECTION("D-1: Delete non-existent key");
    RBTree *root = NULL;
    root = rbtree_insert(root, 10, 10);
    root = rbtree_insert(root,  5,  5);
    root = rbtree_insert(root, 15, 15);

    root = rbtree_remove(root, 99);   /* 99 is not in the tree */

    ASSERT(count_nodes(root) == 3,    "tree still has 3 nodes");
    ASSERT(root->key == 10,           "root unchanged");
    ASSERT(is_valid_rbtree(root),     "tree is a valid RB tree");

    free_tree(root);
}

/* D-2: Delete the only node → tree becomes NULL */
static void test_delete_only_node(void)
{
    SECTION("D-2: Delete the only node");
    RBTree *root = NULL;
    root = rbtree_insert(root, 42, 42);
    root = rbtree_remove(root, 42);

    ASSERT(root == NULL,              "tree is NULL after deleting only node");
}

/* D-3: Delete a red leaf → no fix-up needed */
static void test_delete_red_leaf(void)
{
    SECTION("D-3: Delete a red leaf");
    /* Tree: 10(B), 5(R), 15(R).  Delete 5 (red leaf). */
    RBTree *root = NULL;
    root = rbtree_insert(root, 10, 10);
    root = rbtree_insert(root,  5,  5);
    root = rbtree_insert(root, 15, 15);

    root = rbtree_remove(root, 5);

    ASSERT(count_nodes(root) == 2,         "2 nodes remain");
    ASSERT(root->key == 10,                "root is 10");
    ASSERT(rbtree_search(root, 5) == NULL, "key 5 gone");
    ASSERT(is_valid_rbtree(root),          "tree is a valid RB tree");

    free_tree(root);
}

/* D-4: Delete a black node that has exactly one red child */
static void test_delete_black_node_one_red_child(void)
{
    SECTION("D-4: Delete black node with one red child");
    /*
     * Build: 20, 10, 30, 5
     * After inserts: 20(B), 10(B), 30(B), 5(R).
     * Delete 10(B) which has one red child (5).
     * 5 is transplanted up and recolored black.
     */
    RBTree *root = NULL;
    root = rbtree_insert(root, 20, 20);
    root = rbtree_insert(root, 10, 10);
    root = rbtree_insert(root, 30, 30);
    root = rbtree_insert(root,  5,  5);

    root = rbtree_remove(root, 10);

    ASSERT(count_nodes(root) == 3,           "3 nodes remain");
    ASSERT(rbtree_search(root, 10) == NULL,  "key 10 gone");
    RBTree *n5 = rbtree_search(root, 5);
    ASSERT(n5 != NULL,                       "key 5 still present");
    ASSERT(n5->is_black,                     "5 recolored to black");
    ASSERT(is_valid_rbtree(root),            "tree is a valid RB tree");

    free_tree(root);
}

/* D-5: Delete a node with two children → replaced by in-order successor */
static void test_delete_two_children(void)
{
    SECTION("D-5: Delete node with two children (successor replacement)");
    /*
     * Build: 20, 10, 30, 5, 15, 25, 35.
     * After recoloring: 20(B), 10(B), 30(B), 5(R), 15(R), 25(R), 35(R).
     * Delete 20 (root, two children).  Successor = min of right subtree = 25.
     * 25 replaces root.  25's original position (right child of 30? or left
     * child of 30?) gets fixed.
     */
    RBTree *root = NULL;
    root = rbtree_insert(root, 20, 20);
    root = rbtree_insert(root, 10, 10);
    root = rbtree_insert(root, 30, 30);
    root = rbtree_insert(root,  5,  5);
    root = rbtree_insert(root, 15, 15);
    root = rbtree_insert(root, 25, 25);
    root = rbtree_insert(root, 35, 35);

    root = rbtree_remove(root, 20);

    ASSERT(count_nodes(root) == 6,            "6 nodes remain");
    ASSERT(rbtree_search(root, 20) == NULL,   "key 20 gone");
    ASSERT(rbtree_search(root, 25) != NULL,   "key 25 still present");
    ASSERT(root->key == 25,                   "root is now 25 (in-order successor)");
    ASSERT(is_valid_rbtree(root),             "tree is a valid RB tree");

    free_tree(root);
}

/*
 * D-6: Fix-up: sibling is red
 *
 * To get sibling-is-red after deletion we need:
 *   parent is black, deleted node is black with no children, sibling is red.
 *
 * Construction: insert 20, 10, 30, 25, 35, 5
 *   After balancing: 20(B), 10(B), 30(B), 5(R), 25(R), 35(R).
 *
 * Delete 10 (black leaf, sibling=30 is black with two red children).
 * This actually exercises fix case 4 (sibling black, right nephew red).
 *
 * To get sibling red we need a deeper tree.  Insert additionally 40, 50
 * so that after rebalancing 30 becomes red.
 * Sequence: 20, 10, 30, 25, 35, 40 → 35 triggers RR rotation making
 * 30(B)→35(B) root of sub, 30(R), 40(R).  Then 20 rotates up etc.
 *
 * Easier reliable approach: build the tree from main.c's example which
 * is known to produce a valid tree, then delete nodes and check invariants.
 */
static void test_delete_fixup_red_sibling(void)
{
    SECTION("D-6: Fix-up: sibling is red");
    /*
     * Insert sequence that generates the sibling-red scenario on deletion:
     *   10, 20, 30, 40, 50, 25
     *
     * Trace:
     *  insert 10 → 10(B)
     *  insert 20 → 10(B), 20(R)
     *  insert 30 → RR on 10: 20(B), 10(R), 30(R)
     *  insert 40 → parent=30(R) uncle=10(R) → recolor: 10,30→B, 20→R→B(root)
     *              result: 20(B), 10(B), 30(B), 40(R)
     *  insert 50 → parent=40(R) uncle=NULL(B) → RR on 30: 40(B), 30(R), 50(R)
     *              overall: 20(B), 10(B), 40(B), 30(R), 50(R)
     *  insert 25 → parent=30(R) uncle=50(R) → recolor: 30,50→B, 40→R
     *              40's parent=20(B) → stop. 25 ends up red.
     *              overall: 20(B), 10(B), 40(R), 30(B), 50(B), 25(R)
     *
     * Delete 10 (black leaf, left child of 20):
     *  sibling = 40(R) → sibling-is-red case:
     *    recolor 40→B, 20→R, left-rotate(20)
     *    40 becomes root, 20(R) is left child, new sibling of double-black = 30(B)
     *    30 has left child 25(R), right=NULL.
     *    s_right_blk=true, s_left_blk=false → double-rotate:
     *      recolor 25→B, 30→R, right-rotate(30); new sibling=25(B)
     *      25→parent's color(R), 20→B, left-rotate(20)
     *    child=20(R) → loop exits, 20→B.
     */
    RBTree *root = NULL;
    root = rbtree_insert(root, 10, 10);
    root = rbtree_insert(root, 20, 20);
    root = rbtree_insert(root, 30, 30);
    root = rbtree_insert(root, 40, 40);
    root = rbtree_insert(root, 50, 50);
    root = rbtree_insert(root, 25, 25);

    ASSERT(is_valid_rbtree(root), "tree valid before delete");

    root = rbtree_remove(root, 10);

    ASSERT(count_nodes(root) == 5,            "5 nodes remain");
    ASSERT(rbtree_search(root, 10) == NULL,   "key 10 gone");
    ASSERT(is_valid_rbtree(root),             "tree is a valid RB tree after fixup");

    free_tree(root);
}

/*
 * D-7: Fix-up: sibling black, both nephews black, parent also black
 *      → double-black propagates upward.
 *
 * We need: deleted node is black leaf, parent is black, sibling is black
 * with both children NULL (black).
 *
 * Build: 10(B), 5(B), 15(B)  then delete 5.
 * (Insert 10 → black root. Insert 5 and 15 → both red.
 *  After a 4th insert they all become black via recolor.)
 *
 * Insert: 10, 5, 15, 3 → recolor makes 5,15 black, 10 stays black, 3 red.
 * Now delete 3 (red leaf) → trivial.
 * Then delete 5 (black leaf, sibling=15(B) with no children):
 *   both nephews black → recolor 15→R, propagate up to 10 (root) → stop.
 *   Root is still black.  15 becomes red.
 */
static void test_delete_fixup_propagate_black_parent(void)
{
    SECTION("D-7: Fix-up: sibling black + both nephews black + parent black");
    RBTree *root = NULL;
    root = rbtree_insert(root, 10, 10);
    root = rbtree_insert(root,  5,  5);
    root = rbtree_insert(root, 15, 15);
    root = rbtree_insert(root,  3,  3);

    /* Make 5 a pure black leaf by removing its only child 3 first */
    root = rbtree_remove(root, 3);   /* red leaf — trivial */

    ASSERT(is_valid_rbtree(root), "tree valid before critical delete");

    /* Now delete 5: black leaf, sibling 15 has no children */
    root = rbtree_remove(root, 5);

    ASSERT(count_nodes(root) == 2,           "2 nodes remain");
    ASSERT(rbtree_search(root, 5) == NULL,   "key 5 gone");
    ASSERT(root->is_black,                   "root is still black");
    ASSERT(is_valid_rbtree(root),            "tree is a valid RB tree");

    free_tree(root);
}

/*
 * D-8: Fix-up: sibling black, both nephews black, parent RED
 *      → just recolor parent to black (stops immediately, no propagation).
 *
 * Build: 10, 5, 15, 3, 7, 12, 20, 17
 * After balancing we get a tree where some internal node is red with a
 * black sibling that has no children.  Remove the appropriate leaf to force
 * this scenario.
 *
 * Simpler reliable construction:
 *   20, 10, 30, 5, 15 → after inserts:
 *     20(B), 10(R), 30(B), 5(B), 15(B)   [uncle recolor happened]
 *   actually let's trace carefully:
 *     insert 20 → 20(B)
 *     insert 10 → 20(B), 10(R)
 *     insert 30 → 20(B), 10(R), 30(R)  [parent black, no fix]
 *     insert 5  → parent=10(R), uncle=30(R) → recolor: 10,30→B, 20→R→B(root)
 *                 20(B), 10(B), 30(B), 5(R)
 *     insert 15 → parent=10(B) → no fix: 15(R)
 *                 20(B), 10(B), 30(B), 5(R), 15(R)
 *
 *   Delete 30 (black leaf, left sibling=10(B) with children 5(R),15(R)):
 *     This is case 4 (sibling black, right nephew red) — not what we want.
 *
 *   Instead delete 15 (red leaf) first to make 10 have only one child.
 *   Then structure: 20(B), 10(B), 30(B), 5(R).
 *   Delete 30 → sibling=10(B), 10 has left child 5(R) and no right child.
 *     s_right_blk=true, s_left_blk=false (5 is red) → case 3+4.
 *
 *   Let's target parent-red case differently.  After I-3 recolor (test I-3):
 *   tree is 10(B), 5(B), 15(B), 3(R).  Delete 3, 15, then
 *   try to delete 5: sibling=NULL, that won't work.
 *
 *   Reliable: 30, 15, 45, 10, 20 →
 *     insert 30 → 30(B)
 *     insert 15 → 30(B), 15(R)
 *     insert 45 → 30(B), 15(R), 45(R)
 *     insert 10 → parent=15(R), uncle=45(R) → recolor: 15,45→B, 30→R→B
 *                 30(B), 15(B), 45(B), 10(R)
 *     insert 20 → parent=15(B) → 20(R)
 *                 30(B), 15(B), 45(B), 10(R), 20(R)
 *
 *   Delete 45 (black leaf, sibling=15(B), 15 has children 10(R) and 20(R)):
 *     This is sibling-with-right-nephew-red case (case 4).
 *
 *   We need the sibling to have NO children.
 *   Remove 10 and 20 first (both red, trivial), giving: 30(B), 15(B), 45(B).
 *   Now delete 45: sibling=15(B), no children → both nephews black.
 *   parent=30(B) → recolor 15→R, propagate: child=30, child_parent=NULL → loop stops.
 *   This is case D-7, not D-8.
 *
 *   For D-8 we need parent to be RED.  This requires the grandparent to exist.
 *   Use: 50, 30, 70, 20, 40, 60, 80, 10, 25 →
 *     insert 50 → 50(B)
 *     insert 30 → 50(B), 30(R)
 *     insert 70 → 50(B), 30(R), 70(R)
 *     insert 20 → parent=30(R), uncle=70(R) → recolor: 30,70→B, 50→R→B
 *                 50(B), 30(B), 70(B), 20(R)
 *     insert 40 → parent=30(B) → 40(R)
 *     insert 60 → parent=70(B) → 60(R)
 *     insert 80 → parent=70(B) → 80(R)
 *     insert 10 → parent=20(R), uncle=40(R) → recolor: 20,40→B, 30→R
 *                 30's parent=50(B) → stop
 *                 50(B), 30(R), 70(B), 20(B), 40(B), 60(R), 80(R), 10(R)
 *     insert 25 → parent=20(B) → 25(R)
 *
 *   Now delete 40 (black leaf, parent=30(R), sibling=20(B) has 10(R),25(R)):
 *     child is on right of 30.  brother = 20(B) with nephews 10(R) and 25(R).
 *     This exercises sibling-black-with-right-nephew-red (symmetric).
 *
 *   Let's remove 10 and 25 first, then delete 40:
 *     sibling=20(B), no children.  parent=30(R).
 *     Both nephews black → recolor 20→R, child=30(R) → loop condition fails.
 *     child (30) becomes black.  This IS case D-8!
 */
static void test_delete_fixup_propagate_red_parent(void)
{
    SECTION("D-8: Fix-up: sibling black + both nephews black + parent red");
    RBTree *root = NULL;
    root = rbtree_insert(root, 50, 50);
    root = rbtree_insert(root, 30, 30);
    root = rbtree_insert(root, 70, 70);
    root = rbtree_insert(root, 20, 20);
    root = rbtree_insert(root, 40, 40);
    root = rbtree_insert(root, 60, 60);
    root = rbtree_insert(root, 80, 80);
    root = rbtree_insert(root, 10, 10);
    root = rbtree_insert(root, 25, 25);

    ASSERT(is_valid_rbtree(root), "tree valid before deletes");

    /* Remove leaves to isolate the target scenario */
    root = rbtree_remove(root, 10);   /* red leaf */
    root = rbtree_remove(root, 25);   /* red leaf */

    ASSERT(is_valid_rbtree(root), "tree valid after removing red leaves");

    /* Now 30 is red, 20(B) and 40(B) are its children with no grandchildren.
     * Delete 40: sibling=20(B) no children, parent=30(R) → recolor stop. */
    root = rbtree_remove(root, 40);

    ASSERT(count_nodes(root) == 6,           "6 nodes remain");
    ASSERT(rbtree_search(root, 40) == NULL,  "key 40 gone");
    ASSERT(is_valid_rbtree(root),            "tree is a valid RB tree");

    free_tree(root);
}

/*
 * D-9: Fix-up: sibling black, right nephew red (left nephew any)
 *      → single left rotation on parent.
 *
 * Build: 20, 10, 40, 30, 50
 *   insert 20 → 20(B)
 *   insert 10 → 20(B), 10(R)
 *   insert 40 → 20(B), 10(R), 40(R)
 *   insert 30 → parent=40(R), uncle=10(R) → recolor: 10,40→B, 20→R→B
 *               20(B), 10(B), 40(B), 30(R)
 *   insert 50 → parent=40(B) → 50(R)
 *               20(B), 10(B), 40(B), 30(R), 50(R)
 *
 * Delete 10 (black leaf, parent=20(B), sibling=40(B)):
 *   sibling 40 has children 30(R) and 50(R).
 *   s_right_blk=false (50 is red) → case 4: single left rotate.
 *   40 gets parent's color (B), parent 20→B, right nephew 50→B.
 *   left-rotate(20): 40 becomes new subtree root.
 *   Result: 40(B), 20(B), 50(B), 30(R).
 */
static void test_delete_fixup_single_rotate(void)
{
    SECTION("D-9: Fix-up: sibling black, right nephew red → single left rotate");
    RBTree *root = NULL;
    root = rbtree_insert(root, 20, 20);
    root = rbtree_insert(root, 10, 10);
    root = rbtree_insert(root, 40, 40);
    root = rbtree_insert(root, 30, 30);
    root = rbtree_insert(root, 50, 50);

    ASSERT(is_valid_rbtree(root), "tree valid before delete");

    root = rbtree_remove(root, 10);

    ASSERT(count_nodes(root) == 4,           "4 nodes remain");
    ASSERT(rbtree_search(root, 10) == NULL,  "key 10 gone");
    ASSERT(is_valid_rbtree(root),            "tree is a valid RB tree");

    free_tree(root);
}

/*
 * D-10: Fix-up: sibling black, only left nephew red (right is black/NULL)
 *       → double rotation (right-rotate sibling, then left-rotate parent).
 *
 * Build: 20, 10, 40, 30
 *   insert 20 → 20(B)
 *   insert 10 → 20(B), 10(R)
 *   insert 40 → 20(B), 10(R), 40(R)
 *   insert 30 → parent=40(R), uncle=10(R) → recolor: 10,40→B, 20→R→B
 *               20(B), 10(B), 40(B), 30(R)
 *
 * Delete 10 (black leaf, parent=20(B), sibling=40(B)):
 *   sibling 40 has left child 30(R) and no right child.
 *   s_right_blk=true, s_left_blk=false → case 3:
 *     recolor 30→B, 40→R, right-rotate(40); new sibling = 30(B).
 *   then case 4:
 *     30 gets parent's color, parent 20→B, right nephew (40, now 30's right)→B
 *     left-rotate(20).
 *   Result: 30(B), 20(B), 40(B).
 */
static void test_delete_fixup_double_rotate(void)
{
    SECTION("D-10: Fix-up: sibling black, only left nephew red → double rotate");
    RBTree *root = NULL;
    root = rbtree_insert(root, 20, 20);
    root = rbtree_insert(root, 10, 10);
    root = rbtree_insert(root, 40, 40);
    root = rbtree_insert(root, 30, 30);

    ASSERT(is_valid_rbtree(root), "tree valid before delete");

    root = rbtree_remove(root, 10);

    ASSERT(count_nodes(root) == 3,           "3 nodes remain");
    ASSERT(rbtree_search(root, 10) == NULL,  "key 10 gone");
    ASSERT(root->key == 30,                  "30 is the new root");
    ASSERT(root->is_black,                   "new root is black");
    ASSERT(root->left && root->left->key == 20,  "20 is left child");
    ASSERT(root->right && root->right->key == 40, "40 is right child");
    ASSERT(is_valid_rbtree(root),            "tree is a valid RB tree");

    free_tree(root);
}

/* D-11: Delete all nodes one by one → tree becomes NULL */
static void test_delete_all(void)
{
    SECTION("D-11: Delete all nodes one by one");
    int keys[] = {10, 20, 30, 40, 50, 25, 15, 5, 35, 45};
    int n = (int)(sizeof(keys) / sizeof(keys[0]));

    RBTree *root = NULL;
    for (int i = 0; i < n; i++)
        root = rbtree_insert(root, keys[i], keys[i]);

    ASSERT(count_nodes(root) == n,  "all nodes inserted");
    ASSERT(is_valid_rbtree(root),   "tree valid before sequential delete");

    /* Delete in a scrambled order to hit different structural cases */
    int del_order[] = {25, 10, 45, 30, 5, 50, 15, 40, 20, 35};
    for (int i = 0; i < n; i++) {
        root = rbtree_remove(root, del_order[i]);
        bool ok = (root == NULL) || is_valid_rbtree(root);
        ASSERT(ok, "tree valid after each delete");
    }

    ASSERT(root == NULL, "tree is NULL after all deletions");
}

/* ═══════════════════════════════════════════════════════════════
 * Additional: large-scale stress test
 * ═══════════════════════════════════════════════════════════════ */
static void test_large_insert_delete(void)
{
    SECTION("STRESS: 50-node insert + delete sequence");
    RBTree *root = NULL;
    /* Insert 50 nodes in a pattern that mixes all rotation cases */
    int vals[] = {
        50, 25, 75, 12, 37, 62, 87,  6, 18, 31,
        43, 56, 68, 81, 93,  3,  9, 15, 21, 28,
        34, 40, 46, 53, 59, 65, 71, 78, 84, 90,
        96,  1,  5,  8, 11, 14, 17, 20, 23, 26,
        29, 32, 35, 38, 41, 44, 47, 51, 54, 57
    };
    int n = (int)(sizeof(vals) / sizeof(vals[0]));

    for (int i = 0; i < n; i++) {
        root = rbtree_insert(root, vals[i], vals[i]);
        ASSERT(is_valid_rbtree(root), "tree valid after each insert");
    }
    ASSERT(count_nodes(root) == n,  "all 50 nodes present");

    /* Delete every second value */
    for (int i = 0; i < n; i += 2) {
        root = rbtree_remove(root, vals[i]);
        bool ok = (root == NULL) || is_valid_rbtree(root);
        ASSERT(ok, "tree valid after each delete");
    }

    /* Delete the rest */
    for (int i = 1; i < n; i += 2) {
        root = rbtree_remove(root, vals[i]);
        bool ok = (root == NULL) || is_valid_rbtree(root);
        ASSERT(ok, "tree valid after each delete");
    }
    ASSERT(root == NULL, "tree is NULL after all 50 deletions");
}

/* ═══════════════════════════════════════════════════════════════
 * main
 * ═══════════════════════════════════════════════════════════════ */
int main(void)
{
    printf("╔══════════════════════════════════════════╗\n");
    printf("║   Red-Black Tree — Test Suite            ║\n");
    printf("╚══════════════════════════════════════════╝\n");

    /* Insert tests */
    test_insert_empty();
    test_insert_parent_black();
    test_insert_uncle_red();
    test_insert_ll();
    test_insert_lr();
    test_insert_rr();
    test_insert_rl();
    test_insert_cascade_recolor();

    /* Delete tests */
    test_delete_nonexistent();
    test_delete_only_node();
    test_delete_red_leaf();
    test_delete_black_node_one_red_child();
    test_delete_two_children();
    test_delete_fixup_red_sibling();
    test_delete_fixup_propagate_black_parent();
    test_delete_fixup_propagate_red_parent();
    test_delete_fixup_single_rotate();
    test_delete_fixup_double_rotate();
    test_delete_all();

    /* Stress */
    test_large_insert_delete();

    /* Summary */
    printf("\n══════════════════════════════════════════\n");
    printf("Results: %d passed, %d failed  (total %d)\n",
           g_passed, g_failed, g_passed + g_failed);
    printf("══════════════════════════════════════════\n");

    return g_failed == 0 ? 0 : 1;
}
