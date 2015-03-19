#
# Makefile
# Lab 9
#

CC      = gcc
CFLAGS  = -g -std=c99 -Wall -I/usr/local/djbfft/include
LIBS = -lportaudio -lsndfile -lfftw3 -lncurses

EXE  = convEngine

HDRS = convolve.h

SRCS = convEngine.c convolve.c

all: $(EXE)

$(EXE): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LIBS)

clean:
	rm -f *~ core $(EXE) *.o
	rm -rf delay.dSYM 
