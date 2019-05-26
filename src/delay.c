#include "delay.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct transform_delay {
  int i;
  int buf_n;
  float *buf;
  float r1, r2;
  bool looped;
};

void transform_delay_init(struct transform_delay **p, int delay, float ratio) {
  *p = (struct transform_delay *)malloc(sizeof(struct transform_delay));
  (*p)->i = 0;
  (*p)->r1 = ratio;
  (*p)->r2 = 1.0 - ratio;
  (*p)->buf_n = delay;
  (*p)->buf = malloc(sizeof(float) * delay);
  (*p)->looped = false;
}

float transform_delay(float y, void *data) {
  struct transform_delay *p = (struct transform_delay *)data;

  int i = p->i;
  float yy = (y * p->r1 + p->buf[i] * p->r2);

  p->buf[i] = y;
  i = (i + 1) % p->buf_n;
  if (i == 0) {
    p->looped = true;
  }
  p->i = i;
  if (p->looped)
    return yy;
  return y;
}
