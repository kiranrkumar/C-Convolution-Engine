/*
 ** convolve.h
 **
 ** Kiran Kumar		March 8, 2015
 **
 ** Function that convolves two signals based on the fftw library
 **
 */

#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <fftw3.h>


void convolve(double *sigOne, int lenOne, double *sigTwo, int lenTwo, double **convolvedSig);