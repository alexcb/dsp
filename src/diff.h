#pragma once

struct transform_diff;

void transform_diff_init(struct transform_diff **p, int n);

float transform_diff(float y, void *data);
