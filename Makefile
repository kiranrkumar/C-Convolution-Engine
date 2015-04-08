#
# Makefile
# Lab 9
#

CC      = gcc
CFLAGS  = -g -std=c99 -Wall -I/usr/local/djbfft/include -I/Applications/Max6/max6-sdk-master/c74support/max-includes -I/Applications/Max6/max6-sdk-master/c74support/msp-includes

LIBS = -lportaudio -lsndfile -lfftw3 -lncurses

EXE  = convEngine

HDRS = convolve.h

SRCS = convEngine.c convolve.c testMSPpatch.c

all: $(EXE)

$(EXE): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LIBS)

clean:
	rm -f *~ core $(EXE) *.o
	rm -rf delay.dSYM 
