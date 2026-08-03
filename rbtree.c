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

static void rbtree_print_internal(RBTree* node, const char* prefix, int is_right) {
  if(!node) return;

  char new_prefix[256];
  snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_right ? "│   " : "    ");
  rbtree_print_internal(node->right, new_prefix, 1);

  printf("%s%s[%s] key=%d val=%d\n",
    prefix,
    is_right ? "┌── " : "└── ",
    node->is_black ? "B" : "R",
    node->key,
    node->value);

  rbtree_print_internal(node->left, new_prefix, 0);
}

void rbtree_print(RBTree* node, int depth) {
  (void)depth;
  if(!node) { printf("(empty tree)\n"); return; }

  printf("[ROOT][%s] key=%d val=%d\n",
    node->is_black ? "B" : "R",
    node->key,
    node->value);

  char prefix[256] = "";
  rbtree_print_internal(node->right, prefix, 1);
  rbtree_print_internal(node->left, prefix, 0);
}

bool nodeExists(RBTree* node) {
  return node != NULL;
}

void rbtree_balance(RBTree* node) {
  // TODO: implement balancing
  (void)node;
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
    root = new_node;
  } else if(value < parent->value) {
    parent->left = new_node;
  } else {
    parent->right = new_node;
  }

  rbtree_balance(new_node);

  return root;
}