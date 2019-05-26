#pragma once

struct transform_synth;

void transform_synth_init(struct transform_synth **p, float a, int rate);

float transform_synth(float y, void *data);
