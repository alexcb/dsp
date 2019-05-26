#pragma once

struct transform_delay;

void transform_delay_init(struct transform_delay **p, int delay, float ratio);

float transform_delay(float y, void *data);
