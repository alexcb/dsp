CC=gcc
CCFLAGS=-g -std=gnu11 -I./src
#CCFLAGS=-O3 -std=gnu11 -I./src
#CCFLAGS=-O3 -std=gnu11 -Wall -Werror -I./src
LDFLAGS=-lmpg123 -lmicrohttpd -lpthread -lm -lpulse -lpulse-simple -lcurses

# ifdef USE_PICO
# -lttspico

SRC=$(wildcard src/**/*.c src/*.c)
OBJ=$(SRC:%.c=%.o)

build: dsp

dsp: $(OBJ)
	$(CC) $(CCFLAGS) -o dsp $^ $(LDFLAGS)

# To obtain object files
%.o: %.c
	$(CC) -c $(CCFLAGS) $< -o $@

clean:
	rm -f dsp test $(OBJ) $(TESTOBJ)

reformat:
	./reformat
