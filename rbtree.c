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

static void rbtree_print_internal(RBTree* node, const char* prefix, int is_last) {
  if (!node) return;

  printf("%s%s[%s] key=%d val=%d\n",
    prefix,
    is_last ? "└── " : "├── ",
    node->is_black ? "B" : "R",
    node->key,
    node->value);

  char child_prefix[256];
  snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix, is_last ? "    " : "│   ");

  int has_left = node->left != NULL;
  int has_right = node->right != NULL;

  if (has_left && has_right) {
    rbtree_print_internal(node->left, child_prefix, 0);
    rbtree_print_internal(node->right, child_prefix, 1);
  } else if (has_left) {
    rbtree_print_internal(node->left, child_prefix, 1);
  } else if (has_right) {
    rbtree_print_internal(node->right, child_prefix, 1);
  }
}

void rbtree_print(RBTree* node, int depth) {
  (void)depth;
  if (!node) { printf("(empty tree)\n"); return; }

  printf("[%s] key=%d val=%d\n",
    node->is_black ? "B" : "R",
    node->key,
    node->value);

  int has_left = node->left != NULL;
  int has_right = node->right != NULL;

  if (has_left && has_right) {
    rbtree_print_internal(node->left, "", 0);
    rbtree_print_internal(node->right, "", 1);
  } else if (has_left) {
    rbtree_print_internal(node->left, "", 1);
  } else if (has_right) {
    rbtree_print_internal(node->right, "", 1);
  }
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