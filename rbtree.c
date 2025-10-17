#include <stdio.h>
#include "rbtree.h"

RBTree* rbtree_create_null(int key, int value) {
  RBTree* node = (RBTree*)malloc(sizeof(RBTree));

  if(node) {
    node->key = key;
    node->value = value;
    node->is_black = true;
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
  }

  return node;
}

RBTree* rbtree_create_node(RBTree* node, int key, int value) {
  if(node) {
    node->key = key;
    node->value = value;
    node->is_black = true;
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
  }

  return node;
}

void rbtree_print(RBTree* node, int depth) {
  if(!node) return;
  rbtree_print(node->left, depth + 1);

  for(int i = 0; i < depth; ++i) printf("  ");

  printf("[%s] key = %d, value = %d\n", node->is_black ? "BLACK" : "RED", node->key, node->value);

  rbtree_print(node->right, depth + 1);
}