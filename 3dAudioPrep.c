/*
 ** 3dAudioPrep.c
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
audioSource *createAudioSource(char *filename, int azimuth)
{
	audioSource *newSrc = (audioSource*)malloc(sizeof(audioSource));

	SNDFILE *audioFile;
    SF_INFO audioData;

    if ((audioFile = sf_open(filename, SFM_READ, &audioData)) == NULL ) 
    {
        printf("Error, could not open audio file%s.\n", filename);
        puts(sf_strerror(NULL));
        return NULL;
    }
	newSrc->azimuth = azimuth;
	newSrc->srcFile = audioFile;
	newSrc->srcInfo = &audioData;
	
	return newSrc;
}


/**********************************************************************************
****************************** createImpulseResponse ******************************
***********************************************************************************
** Parameters:
**		leftIRfilename:		file name of the LEFT HRTF/impulse response
**		righttIRfilename:	file name of the RIGHT HRTF/impulse response
***********************************************************************************/
impulseResponse *createImpulseResponse(char *leftIRfilename, char *rightIRfilename)
{
	impulseResponse *newIR = (impulseResponse*)malloc(sizeof(impulseResponse));

	SNDFILE *leftFile, *rightFile;
    SF_INFO leftInfo, rightInfo;
    double *left_Buffer, *right_Buffer;
	int leftLen, rightLen;

	// Open left IR
    if ((leftFile = sf_open(leftIRfilename, SFM_READ, &leftInfo)) == NULL ) 
    {
        printf("Error, could not open left IR%s.\n", leftIRfilename);
        puts(sf_strerror(NULL));
        return NULL;
    }

	// Open right IR    
    if ((rightFile = sf_open(rightIRfilename, SFM_READ, &rightInfo)) == NULL ) 
    {
        printf("Error, could not open left IR%s.\n", rightIRfilename);
        puts(sf_strerror(NULL));
        return NULL;
    }
    
	leftLen = leftInfo.channels * leftInfo.frames;
	rightLen = rightInfo.channels * rightInfo.frames;
	
	// Create IR double buffers
    left_Buffer = (double *)malloc(leftLen * sizeof(double));
    right_Buffer = (double *)malloc(rightLen * sizeof(double));
	
    // Read info from SF_INFO structs into the IR buffers
    sf_readf_double(leftFile, left_Buffer, leftInfo.frames);
    sf_readf_double(rightFile, right_Buffer, rightInfo.frames);
    
	newIR->left_info = leftInfo;
	newIR->right_info = rightInfo;
	
	newIR->left_len = leftLen;
	newIR->right_len = rightLen;
    
    newIR->leftBuffer = left_Buffer;
    newIR->rightBuffer = right_Buffer;
	
	return newIR;
}