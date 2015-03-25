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

/**********************************************************************************
************************************ convolve *************************************
***********************************************************************************
** Parameters:
**		sigOne:			the first of the two audio signals to convolve (e.g. the dry audio)/
**		lenOne:			length of the first audio signal
**		sigTwo:			the second of the two signals to convolve (e.g. the impulse response)
**		lenTwo:			length of the second audio signal
**		convolvedSig:	UNALLOCATED buffer to hold the convolved signal
***********************************************************************************/
void convolve(double *sigOne, int lenOne, double *sigTwo, int lenTwo, double **convolvedSig);