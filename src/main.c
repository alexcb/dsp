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
#include <pthread.h>

#include <pulse/simple.h>
#include <pulse/error.h>
#include <stdio.h>

//#define RATE 44100
#define RATE 16000

#define MEM_SIZE 2500000
#define MAX_BUFF_SIZE 10240

pthread_mutex_t the_lock;
pthread_t audio_thread;
int16_t *audio_buf;
int audio_buf_i;
int audio_buf_n;

int foo_i = 0;
float foo(float y)
{
	foo_i++;
	if( foo_i > 5000 ) {
		foo_i = 0;
	}

	if( foo_i > 2000 ) {
		return 0.f;
	}
	return y;
}

void send_sample(float y)
{
	if( y > 1.0f ) {
		y = 1.0f;
		printf("clipped\n");
	} else if( y < -1.0f ) {
		y = -1.0f;
		printf("clipped\n");
	}
	int16_t yy =  y*32500.0;

	pthread_mutex_lock( &the_lock );

	while( audio_buf_i >= audio_buf_n ) {
		pthread_mutex_unlock( &the_lock );
		//printf("max\n");
		usleep(100);
		pthread_mutex_lock( &the_lock );
	}

	audio_buf[ audio_buf_i ] = yy;
	audio_buf_i++;

	pthread_mutex_unlock( &the_lock );
}

void* audio_run( void *data )
{
	int res;
	int error;

	pa_simple *pa_handle = (pa_simple*) data;

	int min_samples = 1024*32;
	int n = min_samples*sizeof(int16_t);
	char* local_buf = malloc(n);

	for(;;) {
		pthread_mutex_lock( &the_lock );

		if( audio_buf_i < min_samples ) {
			pthread_mutex_unlock( &the_lock );
			usleep(100);
			printf("underrun\n");
			continue;
		}

		memcpy( local_buf, audio_buf, n );
		memmove( audio_buf, ((char*)audio_buf)+n, audio_buf_i*sizeof(int16_t)-n );
		audio_buf_i -= min_samples;
		assert( audio_buf_i >= 0 );
		pthread_mutex_unlock( &the_lock );

		res = pa_simple_write( pa_handle, local_buf, n, &error );
		assert( res == 0 );
	}
}

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

	res = pthread_mutex_init( &the_lock, NULL );
	assert( res == 0 );
	res = pthread_create( &audio_thread, NULL, &audio_run, (void*) pa_handle);
	assert( res == 0 );

	int max_seconds = 10;
	size_t max_samples = RATE * max_seconds;
	size_t buf_size = max_samples * 2;

	audio_buf_i = 0;
	audio_buf_n = buf_size;
	audio_buf = malloc( sizeof(int16_t) * audio_buf_n );

	float tone = 500.0f;
	float angular_frequency1 = tone * 2.0 * M_PI;
	tone += 12.3;
	float angular_frequency2 = tone * 2.0 * M_PI;

	float t = 0.f;
	float dt = 1.0f / (float)RATE;

	float last_y = 0;
	float last_2y = 0;
	while(1) {
		t += dt;
		float y1 = sin( t * angular_frequency1 );
		float y2 = sin( t * angular_frequency2 );

		float y = (y1+y2) / 2.0f;

		y = foo(y);
		send_sample(y);

		//last_y = last_y*0.9 + x1*0.1;
		//last_2y = last_y*0.99 + x2*0.01;

		//int16_t *p = (int16_t*) (buf + i*2);

		//*p = (int)(y*32500.0);

	}
	//	buf_len = 1000*2;
	//	if( buf_len > 0 ) {
	//		res = pa_simple_write( pa_handle, buf, buf_len, &error );
	//		assert( res == 0 );

	//	}
	//}
	//// drain
	//res = pa_simple_drain( pa_handle, &error );
	//assert( res == 0 );

	return 0;
}
