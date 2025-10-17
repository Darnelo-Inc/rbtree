#include <stdio.h>
#include <stdlib.h>
#include "rbtree.h"

int main(void) {
  RBTree* null = rbtree_create_null(10, 20);

  rbtree_print(null, 0);

  free(null);

  return 0;
}