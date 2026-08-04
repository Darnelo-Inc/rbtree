#include <stdio.h>
#include <stdlib.h>
#include "rbtree.h"

int main(void) {
  RBTree* root = NULL;

  /* Asymmetric insertion order: heavy on the right side, sparse on the left. */
  root = rbtree_insert(root,  3,  30);
  root = rbtree_insert(root, 17, 170);
  root = rbtree_insert(root, 42, 420);
  root = rbtree_insert(root, 58, 580);
  root = rbtree_insert(root, 61, 610);
  root = rbtree_insert(root, 73, 730);
  root = rbtree_insert(root,  9,  90);
  root = rbtree_insert(root, 88, 880);
  root = rbtree_insert(root, 95, 950);
  root = rbtree_insert(root, 79, 790);
  root = rbtree_insert(root, 66, 660);

  rbtree_print(root, 0);

  return 0;
}
