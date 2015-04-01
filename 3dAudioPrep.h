/*
 ** 3dAudioPrep.h
 **
 ** Kiran Kumar		March 24, 2015
 **
 ** Functions to prepare audio source and HRTF data for real-time processing
 **
 */
  
#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <fftw3.h>

// Individual audio sources
typedef struct
{
	// PortAudio information for the sound source
	SNDFILE *srcFile;
	SF_INFO *srcInfo;

	// positions in 3-D space
	int azimuth;
	int elevation;
} audioSource;

// Pair of left and right HRTFs/impulse responses
typedef struct
{
	SF_INFO left_info;
	SF_INFO right_info;
	
	int left_len;
	int right_len;
	
	double *leftBuffer;
	double *rightBuffer;
} impulseResponse;

/**********************************************************************************
******************************* createAudioSource *********************************
***********************************************************************************
** Parameters:
**		filename:		file name of the audio source to create
**		azimuth:		angle on the horizontal (azimuth) plane at which the source
							should begin, relative to the listener
***********************************************************************************/
audioSource *createAudioSource(char *filename, int azimuth);


/**********************************************************************************
****************************** createImpulseResponse ******************************
***********************************************************************************
** Parameters:
**		leftIRfilename:		file name of the LEFT HRTF/impulse response
**		righttIRfilename:	file name of the RIGHT HRTF/impulse response
***********************************************************************************/
impulseResponse *createImpulseResponse(char *leftIRfilename, char *rightIRfilename);