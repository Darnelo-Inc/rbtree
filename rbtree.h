#ifndef RBTREE_H
#define RBTREE_H
#include <stdbool.h>
#include <stdlib.h>

typedef struct RBTree {
  int key;
  int value;
  bool is_black;
  struct RBTree *left;
  struct RBTree *right;
  struct RBTree *parent;
} RBTree;

RBTree* rbtree_create_node(int key, int value);
void rbtree_print(RBTree* node, int depth);
bool nodeExists(RBTree* node);
void rbtree_balance(RBTree* node);
RBTree* rbtree_insert(RBTree* root, int key, int value);

#endif