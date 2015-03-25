# Kiran Kumar
#
# Makefile
# Real-Time Convolution Engine
#

CC      = gcc
CFLAGS  = -g -std=c99 -Wall
LIBS = -lportaudio -lsndfile -lfftw3 -lncurses

EXE  = convEngine

HDRS = convolve.h 3dAudioPrep.h

SRCS = convEngine.c convolve.c 3dAudioPrep.c

all: $(EXE)

$(EXE): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LIBS)

clean:
	rm -f *~ core $(EXE) *.o
	rm -rf delay.dSYM 
