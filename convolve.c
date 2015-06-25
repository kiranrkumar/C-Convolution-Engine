/*
 ** convolve.c
 **
 ** Kiran Kumar		March 8, 2015
 **
 ** Function that convolves two signals based on the fftw3 library
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
**		sigOne:			audio signal one (e.g. the dry audio)
**		lenOne:			length in samples - first audio signal
**		sigTwo:			audio signal two (e.g. impulse response)
**		lenTwo:			length in samples - second audio signal
**		convolvedSig:	UNALLOCATED buffer to hold the convolved signal
***********************************************************************************/
void convolve(double *sigOne, int lenOne, double *sigTwo, int lenTwo, double **convolvedSig)
{
	int i;
	int convSigLen = lenOne + lenTwo - 1;
	int nc = convSigLen/2 + 1; // FFTW doesn't output the redundant half of the FFT
	
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

	//create and execute IFFT (backward FFT) plan
	backward_convolution = fftw_plan_dft_c2r_1d(convSigLen, fftMulti, *convolvedSig, FFTW_ESTIMATE);
	fftw_execute(backward_convolution);
	
	
	//destroy plans and free up allocated memory
	fftw_destroy_plan(forward_sigOne);
	fftw_destroy_plan(forward_sigTwo);
	fftw_destroy_plan(backward_convolution);
	
	fftw_free(sigOnePadded);
	fftw_free(sigTwoPadded);
	fftw_free(outfftwOne);
	fftw_free(outfftwTwo);
	fftw_free(fftMulti);
}

/**********************************************************************************
*********************************** deconvolve ************************************
***********************************************************************************
** Parameters:
**		convolvedSig:	the fully convolved signal we wish to deconstruct
**		convSigLen:		length of the fully convolved signal
**		sigOne:			one of the two signals that were convolved to produce convolvedSig
**		lenOne:			length of sigOne
**		outputSig:		UNALLOCATED buffer to hold the output signal
***********************************************************************************/
void deconvolve(double *convolvedSig, int convSigLen, double *sigOne, int lenOne, double **outputSig)
{
}