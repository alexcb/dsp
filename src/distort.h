#pragma once

struct transform_distort;

void transform_distort_init(struct transform_distort **p, float a);

float transform_distort(float y, void *data);
