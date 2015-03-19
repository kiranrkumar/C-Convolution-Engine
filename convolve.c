/*
 ** convolve.c
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
void convolve(double *sigOne, int lenOne, double *sigTwo, int lenTwo, double **convolvedSig)
{
	int i;
	int convSigLen = lenOne + lenTwo - 1;
	int nc = convSigLen/2 + 1;
	
	//Hold the initial signals
	double *sigOnePadded = fftw_malloc(sizeof(double) * convSigLen);
	double *sigTwoPadded = fftw_malloc(sizeof(double) * convSigLen);

	//Forward FFT plans
	fftw_plan forward_sigOne;
	fftw_plan forward_sigTwo;

	//Complex arrays
	fftw_complex *outfftwOne = fftw_malloc(sizeof(fftw_complex) * convSigLen);
	fftw_complex *outfftwTwo = fftw_malloc(sizeof(fftw_complex) * convSigLen);
	fftw_complex *fftMulti = fftw_malloc(sizeof(fftw_complex) * nc);
	
	//Backward (inverse) FFT plan
	fftw_plan backward_convolution;
	
	//zero-pad the two signals
	for (i = 0; i < convSigLen; i++)
	{
		sigOnePadded[i] = (i < lenOne) ? sigOne[i] : 0;
		sigTwoPadded[i] = (i < lenTwo) ? sigTwo[i] : 0;
	}
	
	//create and execute forward FFT plans
	forward_sigOne = fftw_plan_dft_r2c_1d(convSigLen, sigOnePadded, outfftwOne, FFTW_ESTIMATE);
	forward_sigTwo = fftw_plan_dft_r2c_1d(convSigLen, sigTwoPadded, outfftwTwo, FFTW_ESTIMATE);
	
	fftw_execute(forward_sigOne);
	fftw_execute(forward_sigTwo);
	
	//Complex multiplication
	for ( i = 0; i < nc; i++ )
	{
		//real component
		fftMulti[i][0] = outfftwOne[i][0] * outfftwTwo[i][0] - outfftwOne[i][1] * outfftwTwo[i][1];
		//imaginary component
		fftMulti[i][1] = outfftwOne[i][0] * outfftwTwo[i][1] + outfftwOne[i][1] * outfftwTwo[i][0];
	}
	
	*convolvedSig = fftw_malloc(sizeof(double) * convSigLen);

	backward_convolution = fftw_plan_dft_c2r_1d(convSigLen, fftMulti, *convolvedSig, FFTW_ESTIMATE);
	fftw_execute(backward_convolution);
	
	fftw_destroy_plan(forward_sigOne);
	fftw_destroy_plan(forward_sigTwo);
	fftw_destroy_plan(backward_convolution);
	
	fftw_free(sigOnePadded);
	fftw_free(sigTwoPadded);
	fftw_free(outfftwOne);
	fftw_free(outfftwTwo);
	fftw_free(fftMulti);
}