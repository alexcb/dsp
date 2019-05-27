#include "synth.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "kiss_fft.h"

#define N 1024

struct transform_synth {
  kiss_fft_cfg cfg;
  kiss_fft_cpx in[N], out[N];
  int i;
  float tone;
  float t;
  float dt;
  float rate
};

void transform_synth_init(struct transform_synth **p, float a, int rate) {
  *p = (struct transform_synth *)malloc(sizeof(struct transform_synth));

  (*p)->cfg = kiss_fft_alloc(N, 0, 0, 0);
  (*p)->tone = 0.f;
  (*p)->dt = 1.0f / (float)rate;
  (*p)->rate = rate;

  assert(a >= 0.0f);
}

float get_max_tone(kiss_fft_cpx *out, int rate) {
  float m = fabs(out[0].i);
  int j = 0;
  float y;
  for (int i = 0; i < N / 2; i++) {
    // printf("%f %f\n", out[i].r, out[i].i);
    y = out[i].r * out[i].r + out[i].i * out[i].i;
    if (y > m) {
      m = y;
      j = i;
    }
  }

  float hz = (float)(j * rate) / (float)(N);
  return hz;
}

float transform_synth(float y, void *data) {
  struct transform_synth *p = (struct transform_synth *)data;
  p->t += p->dt;

  if (p->i == N) {
    kiss_fft(p->cfg, p->in, p->out);
    p->i = 0;
    p->tone = get_max_tone(p->out, p->rate);
	printf("----------\n");
  } else {
    p->in[p->i].r = y;
    p->in[p->i].i = 0;
    p->i++;
  }

  printf("tone: %f\n", p->tone);
  //return y;
  float angular_frequency = p->tone * 2.0 * M_PI;
  return sin(p->t * angular_frequency);
}
