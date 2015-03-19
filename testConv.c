/*
 * =====================================================================================
 *
 *       Filename:  testConv.c
 *
 *    Description:  Testing convolution of real-time input
 *                  Using functionality from delay.c and shifter.c from MPATE-GE 2618
 *                  - C Programming for Music Technology
 *
 *        Version:  1.0
 *        Created:  02/18/2015 10:06:46
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Kiran Kumar (), kk2916@nyu.edu
 *   Organization:  Steinhardt Music Technology M.M.
 *
 * =====================================================================================
 */

#define SAMPLE_RATE         44100
#define FRAMES_PER_BUFFER   1024
#define NUM_IN_CHANNELS     1
#define NUM_OUT_CHANNELS    2
#define BUFFERSIZE_MAX      (SAMPLE_RATE * 10 * NUM_OUT_CHANNELS) //assume  (IR length + framesPerBuffer) is not longer than 10 seconds

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* for sleep() */
#include <portaudio.h>
#include <sndfile.h>
#include <math.h>
#include <string.h> /* for memset */
#include <stdbool.h>
#include <fftw3.h>
#include "convolve.h"
//#include <ncurses.h>


double RESULTBUFFER[BUFFERSIZE_MAX];
double RESULTBUFFER_LEFT[BUFFERSIZE_MAX];
double RESULTBUFFER_RIGHT[BUFFERSIZE_MAX];

typedef struct {
    //input file - what will be looping on playback
    SNDFILE *infile;
    SF_INFO sf_inInfo;

    //output file - what will be created upon exit
    SNDFILE *outfile;
    SF_INFO sf_outinfo;
    
    //impulse responses
    SF_INFO ir_info;
    SF_INFO irLeft_info;
    SF_INFO irRight_info;

	// hold the input audio for playback
    double file_buff[NUM_OUT_CHANNELS * FRAMES_PER_BUFFER];
    
    double *window; //for windowing the signals prior to FFT

    int ir_len;
    int irLeft_len;
    int irRight_len;
    double *irBuffer;
    double *irLeftBuffer;
    double *irRightBuffer;

	int numConv; //number of convolutions performed

    double ampScal;
    double ampScal2;
} paData;

/*********************************************** 
 ***********************************************
 *********************************************** 
 ********  PORTAUDIO CALLBACK FUNCTION *********
 ***********************************************		
************************************************
************************************************/
static int paCallback( const void *inputBuffer,
        void *outputBuffer, unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData ) 
{
    
	//printf("\nDebug: Begin callback");
    int readcount, i;
    static int resultBufIx = 0;

    static int irIx = 0; //index for where we are in the impulse response
    static bool endOfIR = false;

    //Cast data to appropriate types
    float *out = (float*) outputBuffer;
    paData *data = (paData*) userData;

    // IMPORTANT - Read frames of float type into our buffer for playback
    readcount = sf_readf_double(data->infile, data->file_buff, framesPerBuffer);

    // IMPORTANT - If we're at the end of the file, let's start again
    if ( readcount < framesPerBuffer) 
    {
        sf_seek(data->infile, 0, SEEK_SET);
        readcount = sf_readf_double(data->infile, data->file_buff + readcount, framesPerBuffer - readcount);
    }
    
    /**** Declare variables ****/
    
	int convSigLen = framesPerBuffer + data->ir_len - 1;

    //hold time domain audio and IR signals
    double *in;
    double *inIR;
    double *convolvedSig;

    /**** Crete the input arrays ****/
    
    //Allocate space
    in = fftw_malloc(sizeof(double) * convSigLen );
    inIR = fftw_malloc(sizeof(double) * convSigLen );

	//Store samples into in and inIR
    for (i = 0; i < convSigLen; i++)
    {
        in[i] = (i < framesPerBuffer) ? data->file_buff[i]: 0;
        inIR[i] = (i < data->ir_len) ? data->irBuffer[i] : 0;
    }

    //Check to see if we're at the end of the impulse response (IR)
    if ( (( irIx + framesPerBuffer ) > data->ir_len) ) {
        endOfIR = true;
    }
    
    // Convolve signals via convolution function
    convolve(in, framesPerBuffer, inIR, data->ir_len, &convolvedSig);
    
    /********************************************
     **** COPY DATA INTO APPROPRIATE BUFFERS ****
    *********************************************/
    for (i = 0; i < convSigLen; i++)
    {  
       convolvedSig[i] = convolvedSig[i] / (double) convSigLen;       
       RESULTBUFFER[(2 * resultBufIx) % BUFFERSIZE_MAX] += convolvedSig[i];
       RESULTBUFFER[(2 * resultBufIx + 1) % BUFFERSIZE_MAX] += convolvedSig[i];
       
       
       if (i < framesPerBuffer)
       {
		   //store result buffer values into output
	       out[2*i] = (float) RESULTBUFFER[(2 * resultBufIx) % BUFFERSIZE_MAX];
	       out[2*i + 1] = (float) RESULTBUFFER[(2 * resultBufIx + 1) % BUFFERSIZE_MAX];
	       
	       //zero out the result buffer value so that the index can be used again the next loop around
	       RESULTBUFFER[(2 * resultBufIx) % BUFFERSIZE_MAX] = 0;
	       RESULTBUFFER[(2 * resultBufIx + 1) % BUFFERSIZE_MAX] = 0;
       }
       
       resultBufIx++;
    }


    /**** write the output to a file ****/
    //sf_writef_double(data->outfile, &RESULTBUFFER[(2 * (resultBufIx - convSigLen) ) % BUFFERSIZE_MAX], framesPerBuffer);
    //sf_writef_double(data->outfile, out, framesPerBuffer);
    
	resultBufIx -= (convSigLen - framesPerBuffer);
		
	/**** Free up allocated memory *******/

	//Free memory for all buffers
    fftw_free(in);
    fftw_free(inIR);
    fftw_free(convolvedSig);

    //update indices
    if (endOfIR) {
        irIx = 0;
        endOfIR = false;
    }
    else {
        irIx += framesPerBuffer;
    }

    //DEBUG PRINTS
    //printf("\nEnd of Callback!");
    
    data->numConv++;
    //printf("Buffer size: %lu  IR length: %d   Convolutions Performed: %d",framesPerBuffer, data->ir_len, data->numConv);
    //printf("  Frames Processed: %lu  Seconds Processed: %4.2f\n", framesPerBuffer * data->ir_len * data->numConv, ((float)(framesPerBuffer * data->numConv)/SAMPLE_RATE));

    return paContinue;
}

/*********************************************** 
 ***********************************************
 *********************************************** 
 ***************  MAIN FUNCTION ****************
 ***********************************************
************************************************
************************************************/

int main( int argc, char **argv ) {

    PaStream *stream;
    PaStreamParameters outputParameters;
    PaStreamParameters inputParameters;
    PaError err;
    paData data;

    SNDFILE *irFile, *audioFile;
    SF_INFO irData, audioData;
    double *irBuffer;
    double irLen;

    char *impulseFilename, *audioFilename;


    /* Check arguments */
    if ( argc != 3 ) {
        printf("Usage: %s impulse_response_filename.wav audio_filename.wav\n", argv[0]);
        return EXIT_FAILURE;
    }

    audioFilename = argv[2];
    impulseFilename = argv[1];

    //Open impulse response file
    if ((irFile = sf_open(impulseFilename, SFM_READ, &irData)) == NULL ) 
    {
        printf("Error, could not open impulse file %s.\n", impulseFilename);
        puts(sf_strerror(NULL));
        return EXIT_FAILURE;
    }

    //Open audio file to play
    if ((audioFile = sf_open(audioFilename, SFM_READ, &audioData)) == NULL ) 
    {
        printf("Error, could not open audio file%s.\n", audioFilename);
        puts(sf_strerror(NULL));
        return EXIT_FAILURE;
    }


    /*************************************
    ** Initiate Port Audio data values ***
    **************************************/
    
	// Main audio
    data.sf_inInfo = audioData;
    data.infile = audioFile;

	// Impulse response
    irLen = irData.channels * irData.frames;
    irBuffer = (double *)malloc(irLen * sizeof(double));
    sf_readf_double(irFile, irBuffer, irData.frames);
    data.irBuffer = irBuffer;
    data.ir_len = irLen;

	// Scaling factors
    data.ampScal = .5;
    data.ampScal2 = .1;

    //Print info about audio and impulse response
    printf("Impulse Response\n------------------------\nNumber of input channels: \t%d\nNumber of input frames:\t\t%lld\n\n", irData.channels, irData.frames);
    printf("Audio Signal\n------------------------\nNumber of output channels: \t%d\nNumber of output frames:\t%lld\n\n", audioData.channels, audioData.frames);

    /* Setup sfinfo for output audio file */
    memset(&data.sf_outinfo, 0, sizeof(SF_INFO));
    data.sf_outinfo.samplerate = SAMPLE_RATE;
    data.sf_outinfo.channels = NUM_OUT_CHANNELS;
    data.sf_outinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    if (!sf_format_check(&data.sf_outinfo)) {
        printf("Error: Incorrect audio file format");
        return EXIT_FAILURE;
    }

    /* Open new audio file */
    if (( data.outfile = sf_open( "output.wav", SFM_WRITE, &data.sf_outinfo ) ) == NULL ) {
        printf("Error, couldn't open the file\n");
        return EXIT_FAILURE;
    }


	/* Create Hamming Window */
	int winSize = data.ir_len + FRAMES_PER_BUFFER - 1;
	double *sigWindow = (double *)malloc(winSize * sizeof(double));
	for (int i = 0; i < winSize; i++)
	{
		sigWindow[i] = 0.54 - 0.46*cos(2*M_PI);
	}
	
	data.window = sigWindow;

    /* Initialize PortAudio */
    Pa_Initialize();

    /* Set output stream parameters */
    outputParameters.device = Pa_GetDefaultOutputDevice();
    outputParameters.channelCount = NUM_OUT_CHANNELS;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = 
        Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    /* Set input stream parameters */
    inputParameters.device = Pa_GetDefaultInputDevice();
    inputParameters.channelCount = NUM_IN_CHANNELS;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = 
        Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    /* Open audio stream */
    err = Pa_OpenStream( &stream,
            &inputParameters,
            &outputParameters,
            SAMPLE_RATE, FRAMES_PER_BUFFER, paNoFlag, 
            paCallback, &data );

    if (err != paNoError) {
        printf("PortAudio error: open stream: %s\n", Pa_GetErrorText(err));
    }

    /* Start audio stream */
    err = Pa_StartStream( stream );
    if (err != paNoError) {
        printf(  "PortAudio error: start stream: %s\n", Pa_GetErrorText(err));
    }
    
    data.numConv = 0;

	/***********************************
     *********** USER INPUT*************
     ***********************************/
    /* Allow character input to quit program */
    
    /* Initialize interactive character input */
	/*    initscr(); // Start curses mode
    cbreak();  // Line buffering disabled
    noecho(); // Comment this out if you want to show characters when they are typed
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);*/

    char ch;
    ch = '\0'; /* Init ch to null character */

  /*  mvprintw(6, 6, "Pitch Shift Ratio: %f\n\n[j/k] decreases/increases pitch shift ratio\n" \
            "[q] to quit\n", 2);*/
            
    while (ch != 'q') {
        ch = getchar();
    	//mvprintw(0, 0, "Number of sets of FFT and IFFTs: %d\n", data.numConv);
    }
    
    /* End curses mode  */
    //endwin();
    

	/***********************************
     *********END USER INPUT************
     ***********************************/
     
    err = Pa_StopStream(stream);

    /* Stop audio stream */
    if (err != paNoError) {
        printf(  "PortAudio error: stop stream: %s\n", Pa_GetErrorText(err));
    }
    /* Close audio stream */
    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        printf("PortAudio error: close stream: %s\n", Pa_GetErrorText(err));
    }
    /* Terminate audio stream */
    err = Pa_Terminate();
    if (err != paNoError) {
        printf("PortAudio error: terminate: %s\n", Pa_GetErrorText(err));
    }

    /* Close file */
    sf_close(data.outfile);

    return 0;
}

/*********************************************** 
 ***********************************************
 *********************************************** 
 ****** INITIALIZE IMPULSE RESPONSE DATA *******
 ***********************************************
************************************************
************************************************/

void initIRPair (char *leftFilename, SF_INFO *leftIR, char *rightFilename, SF_INFO *rightIR, paData *data)
{

    SNDFILE *leftIrFile, *rightIrFile;
    double *irLeftBuffer, *irRightBuffer;
    double irLeftLen, irRightLen;

    // Open impulse response (IR) files and read into SF_INFO structs
    
    //   Left
    if ((leftIrFile = sf_open(leftFilename, SFM_READ, leftIR)) == NULL ) 
    {
        printf("Error, could not open impulse file %s.\n", leftFilename);
        puts(sf_strerror(NULL));
        return;
    }
    
    //   Right
    if ((rightIrFile = sf_open(rightFilename, SFM_READ, rightIR)) == NULL ) 
    {
        printf("Error, could not open impulse file %s.\n", rightFilename);
        puts(sf_strerror(NULL));
        return;
    }
    
    // Get IR lengths
    irLeftLen = leftIR->channels * leftIR->frames;
    irRightLen = rightIR->channels * rightIR->frames;
    
    // Create IR double buffers
    irLeftBuffer = (double *)malloc(irLeftLen * sizeof(double));
    irRightBuffer = (double *)malloc(irRightLen * sizeof(double));
    
    // Read info from SF_INFO structs into the IR buffers
    sf_readf_double(leftIrFile, irLeftBuffer, leftIR->frames);
    sf_readf_double(rightIrFile, irRightBuffer, rightIR->frames);
    

	// Store SF_INFO and IR buffer data into PortAudio data struct
    data->irLeft_info = *leftIR;
    data->irRight_info = *rightIR;
    data->irLeftBuffer = irLeftBuffer;
    data->irRightBuffer = irRightBuffer;
	
}
