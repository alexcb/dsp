#include "diff.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct transform_diff {
  float a, b;
  bool ready;
  int n;
  int i;
};

void transform_diff_init(struct transform_diff **p, int n) {
  *p = (struct transform_diff *)malloc(sizeof(struct transform_diff));
  (*p)->ready = false;
  (*p)->i = 0;
  (*p)->n = n;
}

float transform_diff(float y, void *data) {
  struct transform_diff *p = (struct transform_diff *)data;

  if (p->ready == false) {
    if (p->i == 0) {
      p->a = y;
    }
    if (p->i == p->n) {
      p->b = y;
      p->ready = true;
      p->i = 0;
      return y;
    }
    p->i++;
    return y;
  }

  if (p->i == p->n) {
    p->a = p->b;
    p->b = y;
    p->i = 0;
  }

  float dt = (p->b - p->a) / p->n;
  int i = p->i;
  p->i++;
  return p->a + dt * i;
}
