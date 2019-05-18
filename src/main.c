#include <mpg123.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

#include <dirent.h>

#include "sds.h"
#include "timing.h"
#include "math.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include <pulse/simple.h>
#include <pulse/error.h>
#include <stdio.h>

//#define RATE 44100
#define RATE 16000

#define MEM_SIZE 2500000
#define MAX_BUFF_SIZE 10240

int main(int argc, char *argv[])
{
	int char_start;
	int char_end;

	if( argc == 1 ) {
		char_start = 0;
		char_end = 37;
	} else if( argc == 3  ) {
		char_start = atoi(argv[1]);
		char_end = atoi(argv[2]);
	} else {
		fprintf(stderr, "usage: %s <char start> <char end>\n", argv[0]);
		return 1;
	}

	if( char_end <= char_start || char_end < 0 || char_end > 37) {
		fprintf(stderr, "<char start> must be less than <char end> (from 0 to 37)\n");
		return 1;
	}

	int res;
	int error;

	srand( get_current_time_ms() );

	static const pa_sample_spec ss = {
		.format = PA_SAMPLE_S16LE,
		.rate = RATE,
		.channels = 1
	};

	pa_simple *pa_handle;
	pa_handle = pa_simple_new(NULL, "alexplayer", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error);
	if( pa_handle == NULL ) {
		fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
		assert( 0 );
	}

	int max_seconds = 10;
	size_t max_samples = RATE * max_seconds;
	size_t buf_size = max_samples * 2;

	size_t buf_len;
	char *buf = malloc( buf_size );
	memset( buf, 0, buf_size );


	float tone = 500.0f;
	float angular_frequency1 = tone * 2.0 * M_PI;

	float t = 0.f;
	float dt = 1.0f / (float)RATE;
	while(1) {
		for( int i = 0; i < 1000; i++ ) {
			t += dt;
			float x1 = sin( t * angular_frequency1 );
			//float x2 = cos( t * angular_frequency2 );
			//x2 = 1.0f;
			//int16_t xx = x1 * 32500;
			int16_t xx = (int)(x1*1000.0); // * 32500;
			int16_t *p = (int16_t*) (buf + i*2);

			*p = xx;
		}
		buf_len = 1000*2;
		if( buf_len > 0 ) {
			res = pa_simple_write( pa_handle, buf, buf_len, &error );
			assert( res == 0 );

		}
	}
	// drain
	res = pa_simple_drain( pa_handle, &error );
	assert( res == 0 );

	return 0;
}
