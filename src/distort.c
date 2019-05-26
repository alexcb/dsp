#include "distort.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct transform_distort {
  float a;
};

void transform_distort_init(struct transform_distort **p, float a) {
  *p = (struct transform_distort *)malloc(sizeof(struct transform_distort));
  assert(a >= 0.0f);
  (*p)->a = -a;
}

float transform_distort(float y, void *data) {
  assert(y <= 1.0f);
  struct transform_distort *p = (struct transform_distort *)data;

  float s = y > 0 ? 1.0f : -1.0f;

  return s * (1 - pow(M_E, p->a * fabs(y)));
}
