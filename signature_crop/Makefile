CC=gcc
CFLAGS = -Wall -Wextra
APPFLAGS = -DUSE_LOCAL_TIFF -DDEBUG
MAGICK_LIBS = `pkg-config --cflags --libs MagickWand`

# DBG enabled by default
DBG ?= 1
ifeq ($(DBG), 1)
CFLAGS := $(CFLAGS) -g
APPFLAGS := $(APPFLAGS) -DDEBUG
else
CFLAGS := $(CFLAGS) -O3
endif

OBJS = utils.o log.o

log.o: log.h
	$(CC) $(CFLAGS) -c log.c -o $@

utils.o: utils.h
	$(CC) $(CFLAGS) $(APPFLAGS) -c utils.c -o $@ -L $(MAGICK_LIBS)

omr: $(OBJS)
	$(CC) $(CFLAGS) $(APPFLAGS) omr.c -o $@ $(OBJS) -L $(MAGICK_LIBS)
	if [ $(DBG) -eq 0 ]; then strip $@; fi

# just an util for tests
tiffg4:
	$(CC) $(CFLAGS) tiffg4.c -o $@ -L $(MAGICK_LIBS)

potracer: $(OBJS)
	$(CC) $(CFLAGS) potracer.c -o $@

clean:
	rm -f *.o omr tiffg4

all: clean omr
