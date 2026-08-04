#include <stdio.h>
#include <string.h>
#include "rbtree.h"

RBTree* rbtree_create_node(int key, int value) {
  RBTree* node = (RBTree*)malloc(sizeof(RBTree));

  if(node) {
    node->key = key;
    node->value = value;
    node->is_black = false;
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
  }

  return node;
}

void rbtree_print(RBTree* root, int depth_unused) {
  (void)depth_unused;
  if (!root) { printf("(empty tree)\n"); return; }

  typedef struct { RBTree* node; int idx; } E;

  E *cur = malloc(4096 * sizeof(E));
  E *nxt = malloc(4096 * sizeof(E));
  E *lvl[32];
  int lvl_n[32];
  int D = 0;

  cur[0].node = root; cur[0].idx = 0;
  int cur_n = 1;

  while (cur_n > 0) {
    lvl[D] = malloc(cur_n * sizeof(E));
    memcpy(lvl[D], cur, cur_n * sizeof(E));
    lvl_n[D] = cur_n;
    D++;

    int nxt_n = 0;
    for (int i = 0; i < cur_n; i++) {
      E e = cur[i];
      if (e.node->left)  { nxt[nxt_n].node = e.node->left;  nxt[nxt_n].idx = 2*e.idx+1; nxt_n++; }
      if (e.node->right) { nxt[nxt_n].node = e.node->right; nxt[nxt_n].idx = 2*e.idx+2; nxt_n++; }
    }
    E *tmp = cur; cur = nxt; nxt = tmp;
    cur_n = nxt_n;
  }

  const int NODE_W = 6;   /* "[B]58" = up to 6 chars for 2-digit key */
  const int CELL   = NODE_W + 2;

  char line[8192];
  for (int L = 0; L < D; L++) {
    int slot_w = CELL * (1 << (D - 1 - L));

    memset(line, ' ', sizeof(line));
    int last = 0;

    for (int i = 0; i < lvl_n[L]; i++) {
      E e = lvl[L][i];
      int pos   = e.idx - ((1 << L) - 1);
      int start = pos * slot_w + slot_w / 2 - NODE_W / 2;

      char buf[32];
      int len = snprintf(buf, sizeof(buf), "[%c]%d",
                         e.node->is_black ? 'B' : 'R', e.node->key);
      for (int c = 0; c < len; c++) line[start + c] = buf[c];
      if (start + len > last) last = start + len;
    }

    line[last] = '\0';
    printf("%s\n", line);
    if (L < D - 1) printf("\n");
    free(lvl[L]);
  }

  free(cur);
  free(nxt);
}

bool nodeExists(RBTree* node) {
  return node != NULL;
}

static void left_rotate(RBTree* node) {
  RBTree* y = node->right;
  node->right = y->left;
  if (y->left != NULL)
    y->left->parent = node;
  y->parent = node->parent;
  if (node->parent != NULL) {
    if (node == node->parent->left)
      node->parent->left = y;
    else
      node->parent->right = y;
  }
  y->left = node;
  node->parent = y;
}

static void right_rotate(RBTree* node) {
  RBTree* y = node->left;
  node->left = y->right;
  if (y->right != NULL)
    y->right->parent = node;
  y->parent = node->parent;
  if (node->parent != NULL) {
    if (node == node->parent->right)
      node->parent->right = y;
    else
      node->parent->left = y;
  }
  y->right = node;
  node->parent = y;
}

void rbtree_balance(RBTree* node) {
  RBTree* uncle;

  while (node->parent != NULL && !node->parent->is_black) {
    if (node->parent == node->parent->parent->left) {
      uncle = node->parent->parent->right;

      if (uncle != NULL && !uncle->is_black) {
        node->parent->is_black = true;
        uncle->is_black = true;
        node->parent->parent->is_black = false;
        node = node->parent->parent;
      } else {
        if (node == node->parent->right) {
          node = node->parent;
          left_rotate(node);
        }
        node->parent->is_black = true;
        node->parent->parent->is_black = false;
        right_rotate(node->parent->parent);
      }
    } else {
      uncle = node->parent->parent->left;

      if (uncle != NULL && !uncle->is_black) {
        node->parent->is_black = true;
        uncle->is_black = true;
        node->parent->parent->is_black = false;
        node = node->parent->parent;
      } else {
        if (node == node->parent->left) {
          node = node->parent;
          right_rotate(node);
        }

        node->parent->is_black = true;
        node->parent->parent->is_black = false;
        left_rotate(node->parent->parent);
      }
    }
  }

  RBTree* root = node;
  while (root->parent != NULL)
    root = root->parent;
  root->is_black = true;
}

RBTree* rbtree_insert(RBTree* root, int key, int value) {
  RBTree* current = root;
  RBTree* parent = NULL;

  while(nodeExists(current)) {
    parent = current;
    if(value < current->value)
      current = current->left;
    else
      current = current->right;
  }

  RBTree* new_node = rbtree_create_node(key, value);
  new_node->parent = parent;

  if(!nodeExists(parent)) {
    new_node->is_black = true;
    return new_node;
  } else if(value < parent->value) {
    parent->left = new_node;
  } else {
    parent->right = new_node;
  }

  rbtree_balance(new_node);

  root = new_node;
  while (root->parent != NULL)
    root = root->parent;

  return root;
}

RBTree* rbtree_search(RBTree* root, int key) {
  RBTree* current = root;
  while (current != NULL) {
    if (key == current->key) return current;
    else if (key < current->key) current = current->left;
    else current = current->right;
  }
  return NULL;
}

static RBTree* rbtree_get_min(RBTree* node) {
  while (node->left != NULL)
    node = node->left;
  return node;
}

static int get_children_count(RBTree* node) {
  int count = 0;
  if (nodeExists(node->left)) count++;
  if (nodeExists(node->right)) count++;
  return count;
}

static RBTree* get_child_or_null(RBTree* node) {
  return nodeExists(node->left) ? node->left : node->right;
}

static void transplant_node(RBTree** root, RBTree* to_node, RBTree* from_node) {
  if (to_node->parent == NULL)
    *root = from_node;
  else if (to_node == to_node->parent->left)
    to_node->parent->left = from_node;
  else
    to_node->parent->right = from_node;

  if (from_node != NULL)
    from_node->parent = to_node->parent;
}

static RBTree* fix_rules_after_removal(RBTree* root, RBTree* child,
                                       RBTree* child_parent) {
  while (child_parent != NULL && (child == NULL || child->is_black)) {

    if (child == child_parent->left) {
      RBTree* brother = child_parent->right;

      if (brother != NULL && !brother->is_black) {
        brother->is_black = true;
        child_parent->is_black = false;
        left_rotate(child_parent);
        brother = child_parent->right;
      }

      bool s_left_blk = (brother == NULL || brother->left == NULL || brother->left->is_black);
      bool s_right_blk = (brother == NULL || brother->right == NULL || brother->right->is_black);

      if (s_left_blk && s_right_blk) {
        if (brother) brother->is_black = false;
        child = child_parent;
        child_parent = child->parent;
      } else {
        if (s_right_blk) {
          if (brother && brother->left) brother->left->is_black = true;
          if (brother) brother->is_black = false;
          right_rotate(brother);
          brother = child_parent->right;
          s_right_blk = false;
          (void)s_right_blk;
        }
        if (brother) brother->is_black = child_parent->is_black;
        child_parent->is_black = true;
        if (brother && brother->right) brother->right->is_black = true;
        left_rotate(child_parent);
        break;
      }

    } else {
      RBTree* brother = child_parent->left;

      if (brother != NULL && !brother->is_black) {
        brother->is_black = true;
        child_parent->is_black = false;
        right_rotate(child_parent);
        brother = child_parent->left;
      }

      bool s_left_blk = (brother == NULL || brother->left == NULL || brother->left->is_black);
      bool s_right_blk = (brother == NULL || brother->right == NULL || brother->right->is_black);

      if (s_left_blk && s_right_blk) {
        if (brother) brother->is_black = false;
        child = child_parent;
        child_parent = child->parent;
      } else {
        if (s_left_blk) {
          if (brother && brother->right) brother->right->is_black = true;
          if (brother) brother->is_black = false;
          left_rotate(brother);
          brother = child_parent->left;
        }
        if (brother) brother->is_black = child_parent->is_black;
        child_parent->is_black = true;
        if (brother && brother->left) brother->left->is_black = true;
        right_rotate(child_parent);
        break;
      }
    }
  }

  if (child) child->is_black = true;

  if (root) {
    while (root->parent != NULL)
      root = root->parent;
  }
  return root;
}

RBTree* rbtree_remove(RBTree* root, int key) {
  RBTree* node_to_delete = rbtree_search(root, key);
  if (node_to_delete == NULL) return root;

  bool removed_black = node_to_delete->is_black;
  RBTree* child;
  RBTree* child_parent;

  if (get_children_count(node_to_delete) < 2) {
    child = get_child_or_null(node_to_delete);
    child_parent = node_to_delete->parent;
    transplant_node(&root, node_to_delete, child);
    free(node_to_delete);
  } else {
    RBTree* min_node = rbtree_get_min(node_to_delete->right);

    node_to_delete->key = min_node->key;
    node_to_delete->value = min_node->value;

    removed_black = min_node->is_black;
    child = get_child_or_null(min_node);
    child_parent = min_node->parent;

    transplant_node(&root, min_node, child);
    free(min_node);
  }

  if (removed_black)
    root = fix_rules_after_removal(root, child, child_parent);

  return root;
}