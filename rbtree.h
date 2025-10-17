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

RBTree* rbtree_create_null(int key, int value);
RBTree* rbtree_create_node(RBTree* node, int key, int value);
void rbtree_print(RBTree* node, int depth);

#endif