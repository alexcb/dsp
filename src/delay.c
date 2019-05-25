#include "delay.h"

#include <stdlib.h>
#include <string.h>

int i = 0;
int buf_n = 10000;
float *buf = NULL;

float transform_delay(float y)
{
	if( buf == NULL ) {
		buf = malloc(sizeof(float)*buf_n);
		memset( buf, 0, sizeof(float)*buf_n);
	}

	float yy = (y + buf[i]) / 2.f;

	buf[i] = y;
	i = (i+1) % buf_n;
	return yy;
}
