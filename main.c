#include <stdio.h>
#include <stdlib.h>
#include "rbtree.h"

int main(void) {
  RBTree* root = NULL;

  root = rbtree_insert(root, 50, 500);
  root = rbtree_insert(root, 25, 250);
  root = rbtree_insert(root, 75, 750);
  root = rbtree_insert(root, 10, 100);
  root = rbtree_insert(root, 35, 350);
  root = rbtree_insert(root, 60, 600);
  root = rbtree_insert(root, 90, 900);
  root = rbtree_insert(root, 5, 50);
  root = rbtree_insert(root, 15, 150);
  root = rbtree_insert(root, 30, 300);
  root = rbtree_insert(root, 40, 400);
  root = rbtree_insert(root, 55, 550);
  root = rbtree_insert(root, 70, 700);
  root = rbtree_insert(root, 85, 850);
  root = rbtree_insert(root, 95, 950);

  rbtree_print(root, 0);

  return 0;
}
