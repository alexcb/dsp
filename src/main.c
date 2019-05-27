#include <assert.h>
#include <mpg123.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <dirent.h>

#include "math.h"

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <pulse/error.h>
#include <pulse/simple.h>
#include <stdio.h>

// transforms
#include "delay.h"
#include "diff.h"
#include "distort.h"
#include "synth.h"

//#define RATE 44100
#define RATE 16000

#define MEM_SIZE 2500000
#define MAX_BUFF_SIZE 10240

#define BUF_SIZE 8
#define READ_BUF_SIZE BUF_SIZE
#define OUTPUT_BUF_SIZE BUF_SIZE

pthread_mutex_t the_lock;
pthread_t audio_thread;
int16_t *audio_buf;
int audio_buf_i;
int audio_buf_n;

mpg123_handle *mh;

typedef float (*transformer)(float y, void *data);

struct effect {
  transformer fn;
  void *data;
  struct effect *next;
};

struct effect *root_effect;

float transform_sample(float y) {
  for (struct effect *p = root_effect; p != NULL; p = p->next) {
    y = p->fn(y, p->data);
  }
  return y;
}

void add_effect(transformer fn, void *data) {
  struct effect *q;
  struct effect *p = malloc(sizeof(struct effect));
  p->fn = fn;
  p->data = data;
  p->next = NULL;
  if (root_effect == NULL) {
    root_effect = p;
    return;
  }
  q = root_effect;
  for (;;) {
    if (q->next == NULL) {
      q->next = p;
      return;
    }
    q = q->next;
  }
}

void send_sample(float y) {
  if (y > 1.0f) {
    y = 1.0f;
    printf("clipped\n");
  } else if (y < -1.0f) {
    y = -1.0f;
    printf("clipped\n");
  }
  int16_t yy = y * 32500.0;

  pthread_mutex_lock(&the_lock);

  while (audio_buf_i >= audio_buf_n) {
    pthread_mutex_unlock(&the_lock);
    // printf("no space\n");
    usleep(1000);
    pthread_mutex_lock(&the_lock);
  }

  audio_buf[audio_buf_i] = yy;
  audio_buf_i++;

  pthread_mutex_unlock(&the_lock);
}

void *audio_run(void *data) {
  int res;
  int error;

  pa_simple *pa_handle = (pa_simple *)data;

  int min_samples = OUTPUT_BUF_SIZE;
  int n = min_samples * sizeof(int16_t);
  char *local_buf = malloc(n);

  for (;;) {
    pthread_mutex_lock(&the_lock);

    if (audio_buf_i < min_samples) {
      pthread_mutex_unlock(&the_lock);
      // usleep(10);
      printf("underrun\n");
      continue;
    }

    memcpy(local_buf, audio_buf, n);
    memmove(audio_buf, ((char *)audio_buf) + n,
            audio_buf_i * sizeof(int16_t) - n);
    audio_buf_i -= min_samples;
    assert(audio_buf_i >= 0);
    pthread_mutex_unlock(&the_lock);

    res = pa_simple_write(pa_handle, local_buf, n, &error);
    assert(res == 0);
  }

  // drain
  res = pa_simple_drain(pa_handle, &error);
  assert(res == 0);

  return NULL;
}

void with_mp3(const char *path) {
  int i;
  int res;
  int fd;

  // mp3 decoder init
  mpg123_init();
  mh = mpg123_new(NULL, NULL);
  mpg123_format_none(mh);
  mpg123_format(mh, RATE, MPG123_MONO, MPG123_ENC_SIGNED_16);

  fd = open(path, O_RDONLY);
  assert(fd >= 0);
  res = mpg123_open_fd(mh, fd);
  assert(res == 0);

  int n = 102400;
  char *decode_buffer = malloc(n);
  size_t decoded_size;

  for (;;) {
    res = mpg123_read(mh, (unsigned char *)decode_buffer, n, &decoded_size);
    switch (res) {
    case MPG123_OK:
      break;
    case MPG123_NEW_FORMAT:
      break;
    case MPG123_DONE:
      goto done;
      break;
    default:
      break;
    }
    for (i = 0; i < decoded_size; i += 2) {
      uint8_t ll = ((uint8_t *)decode_buffer)[i];
      int8_t hh = ((int8_t *)decode_buffer)[i + 1];
      float y = (hh << 8) + ll;
      y /= 32500.0;
      send_sample(transform_sample(y));
    }
  }
done:
  printf("done!\n");

  return;
}

void with_sine_wave() {
  float tone = 500.0f;
  float angular_frequency1 = tone * 2.0 * M_PI;
  tone += 12.3;
  float angular_frequency2 = tone * 2.0 * M_PI;

  float t = 0.f;
  float dt = 1.0f / (float)RATE;

  float last_y = 0;
  float last_2y = 0;
  while (1) {
    t += dt;
    float y1 = sin(t * angular_frequency1);
    float y2 = sin(t * angular_frequency2);
    y2 = 0;

    float y = (y1 + y2) / 2.0f;

    send_sample(transform_sample(y));
  }
}

void with_input(const pa_sample_spec *ss) {

  pa_simple *s = NULL;
  int ret = 1;
  int error;

  if (!(s = pa_simple_new(NULL, "alexplayer", PA_STREAM_RECORD, NULL, "record",
                          ss, NULL, NULL, &error))) {
    fprintf(stderr, __FILE__ ": pa_simple_new() failed: %s\n",
            pa_strerror(error));
    assert(0);
  }

  for (;;) {
    uint8_t buf[READ_BUF_SIZE];

    /* Record some data ... */
    if (pa_simple_read(s, buf, sizeof(buf), &error) < 0) {
      fprintf(stderr, __FILE__ ": pa_simple_read() failed: %s\n",
              pa_strerror(error));
      assert(0);
    }

    for (int i = 0; i < sizeof(buf); i += 2) {
      uint8_t ll = ((uint8_t *)buf)[i];
      int8_t hh = ((int8_t *)buf)[i + 1];
      float y = (hh << 8) + ll;
      y /= 32500.0;
      // send_sample(transform_sample(y));
      send_sample(transform_sample(y));
    }

    /* And write it to STDOUT */
    // if (loop_write(STDOUT_FILENO, buf, sizeof(buf)) != sizeof(buf)) {
    //    fprintf(stderr, __FILE__": write() failed: %s\n", strerror(errno));
    //    goto finish;
    //}
  }
}

void chain_synth(int n) {
  struct transform_synth *synth;
  transform_synth_init(&synth, 1.2f, RATE, n);
  add_effect(transform_synth, synth);
}

void chain_delay(int n, float ratio) {
  struct transform_delay *delay;
  transform_delay_init(&delay, n, ratio);
  add_effect(transform_delay, delay);
}

void chain_diff(int n) {
  struct transform_diff *diff;
  transform_diff_init(&diff, n);
  add_effect(transform_diff, diff);
}

void chain_distort(float a) {
  struct transform_distort *distort;
  transform_distort_init(&distort, a);
  add_effect(transform_distort, distort);
}

int main(int argc, char *argv[]) {
  const char *path = NULL;

  if (argc == 1) {
    // pass
  } else if (argc == 2) {
    path = argv[1];
  } else {
    fprintf(stderr, "usage: %s <path>\n", argv[0]);
    return 1;
  }

  int res;
  int error;

  // srand( get_current_time_ms() );

  const pa_sample_spec ss = {
      .format = PA_SAMPLE_S16LE, .rate = RATE, .channels = 1};

  pa_simple *pa_handle;
  pa_handle = pa_simple_new(NULL, "alexplayer", PA_STREAM_PLAYBACK, NULL,
                            "playback", &ss, NULL, NULL, &error);
  if (pa_handle == NULL) {
    fprintf(stderr, __FILE__ ": pa_simple_new() failed: %s\n",
            pa_strerror(error));
    assert(0);
  }

  res = pthread_mutex_init(&the_lock, NULL);
  assert(res == 0);
  res = pthread_create(&audio_thread, NULL, &audio_run, (void *)pa_handle);
  assert(res == 0);

  // init buffer space
  int max_seconds = 10;
  size_t max_samples = RATE * max_seconds;
  size_t buf_size = max_samples * 2;

  audio_buf_i = 0;
  audio_buf_n = buf_size;
  audio_buf = malloc(sizeof(int16_t) * audio_buf_n);

  root_effect = NULL;

  chain_synth(1);

  // chain together transforms
  for (int i = 0; i < 10; i++) {
    //chain_delay(rand() % 1000 + 10, 0.50);
    //chain_diff(2);
  }

   //chain_delay(1000, 0.80);
   //chain_delay(5000, 0.20);
  // chain_diff(2);
  // chain_diff(5);

  // chain_distort( 50 );
  // chain_diff(2);
  // chain_delay(123, 0.10);
  // chain_delay(847, 0.70);

  // chain_synth(40);

  with_input(&ss);

  if (path) {
    with_mp3(path);
  } else {
    with_sine_wave();
  }

  pthread_join(audio_thread, NULL);
  return 0;
}

// TODO watch
// weird pedals for ideas: https://www.youtube.com/watch?v=lsYmvfwFmzQ
//
// TODO figure out the tone of the incoming signal, then play a sinewave of the
// same freq
//
// TODO delay with randomized delay speeds/volumes
