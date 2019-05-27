#include "synth.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "kiss_fft.h"

#define N 512
//#define N 1024
//#define N 64

struct hz {
  float mag;
  float hz;
};

struct transform_synth {
  kiss_fft_cfg cfg;
  kiss_fft_cpx in[N], out[N];
  int i;
  float tone;
  float t;
  float dt;
  float rate;
  struct hz sorted_tones[N];
  int keep;
};

void transform_synth_init(struct transform_synth **p, float a, int rate,
                          int keep) {
  *p = (struct transform_synth *)malloc(sizeof(struct transform_synth));

  (*p)->cfg = kiss_fft_alloc(N, 0, 0, 0);
  (*p)->tone = 0.f;
  (*p)->dt = 1.0f / (float)rate;
  (*p)->rate = rate;
  (*p)->keep = keep;

  assert(a >= 0.0f);
}

int mag_cmp(struct hz *a, struct hz *b) {
  if (a->mag < b->mag) {
    return 1;
  }
  if (a->mag > b->mag) {
    return -1;
  }
  return 0;
}

float get_max_tones(kiss_fft_cpx *out, int rate, struct hz *data) {
  float m[N];
  float y;

  int n = N / 2 - 1;

  for (int i = 0; i < n; i++) {
    data[i].mag = out[i].r * out[i].r + out[i].i * out[i].i;
    data[i].hz = (float)(i * rate) / (float)(N);
  }

  qsort(data, n, sizeof(struct hz),
        (int (*)(const void *, const void *))mag_cmp);
}

float transform_synth(float y, void *data) {
  struct transform_synth *p = (struct transform_synth *)data;
  p->t += p->dt;

  if (p->i == N) {
    kiss_fft(p->cfg, p->in, p->out);
    p->i = 0;
    get_max_tones(p->out, p->rate, p->sorted_tones);
  } else {
    p->in[p->i].r = y;
    p->in[p->i].i = 0;
    p->i++;
  }

  // return y;
  float m;
  float angular_frequency;
  y = 0.0f;
  float mm = 0.0f;
  for (int i = 0; i < p->keep; i++) {
    angular_frequency = p->sorted_tones[i].hz * 2.0 * M_PI;
    y += sin(p->t * angular_frequency);
  }
  return y / p->keep;
}
