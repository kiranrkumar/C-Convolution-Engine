/*
 * =====================================================================================
 *
 *       Filename:  convEngine.c
 *
 *    Description:  Testing convolution of real-time input
 *                  
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
#define FRAMES_PER_BUFFER   64
#define NUM_IN_CHANNELS     1
#define NUM_OUT_CHANNELS    2
#define BUFFERSIZE_MAX      (SAMPLE_RATE * 10 * NUM_OUT_CHANNELS) //assume  (IR length + framesPerBuffer) is not longer than 10 seconds
#define AZI_MIN				0
#define	AZI_MAX				359
#define AZI_INC				5
#define ELE_MIN				-90
#define ELE_MAX				90
#define ELE_INC				5

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
#include <ncurses.h>

double RESULTBUFFER_LEFT[BUFFERSIZE_MAX];
double RESULTBUFFER_RIGHT[BUFFERSIZE_MAX];



typedef struct 
{
    //input file - what will be looping on playback
    SNDFILE *infile;
    SF_INFO sf_inInfo;

    //output file - what will be created upon exit
    SNDFILE *outfile;
    SF_INFO sf_outinfo;
    
    //impulse response data
    SF_INFO irLeft_info;
    SF_INFO irRight_info;

    int irLeft_len;
    int irRight_len;
    
    double *irLeftBuffer;
    double *irRightBuffer;

	// positions in 3-D space
	int azimuth; //should be between 0 and 360
	int elevation; //should be between -90 and 90 (theoretically)

	// hold the input audio for playback
    double file_buff[NUM_IN_CHANNELS * FRAMES_PER_BUFFER];

	int numConv; //number of convolutions performed

    double ampScal;
    double ampScal2;
} paData;

void initAudioData(char *filename, paData *data, int az, int elev);
void initIRPair(char *leftFilename, SF_INFO *leftIR, char *rightFilename, SF_INFO *rightIR, paData *data);

void initBuffers()
{
	for (int i = 0; i < BUFFERSIZE_MAX; i++)
	{
		RESULTBUFFER_LEFT[i] = 0;
		RESULTBUFFER_RIGHT[i] = 0;
	}
}

/*********************************************** 
 *********************************************** 
 ********  PORTAUDIO CALLBACK FUNCTION *********
 ***********************************************		
 ***********************************************/
static int paCallback( const void *inputBuffer,
        void *outputBuffer, unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData ) 
{
    
    int readcount, i;
    static int resultIxLeft = 0, resultIxRight = 0;

    static int irIx = 0; //index for where we are in the impulse response
    static bool endOfIR = false;

    //Cast data to appropriate types
    float *out = (float*) outputBuffer;
    paData *data = (paData*) userData;

    // IMPORTANT - Read frames of double type into our buffer for playback
    readcount = sf_readf_double(data->infile, data->file_buff, framesPerBuffer);

    // IMPORTANT - If we're at the end of the file, let's start again
    if ( readcount < framesPerBuffer) 
    {
        sf_seek(data->infile, 0, SEEK_SET);
        readcount = sf_readf_double(data->infile, data->file_buff + readcount, framesPerBuffer - readcount);
    }
    
    /**** Declare variables ****/
    
	int convSigLenLeft = framesPerBuffer + data->irLeft_len - 1;
	int convSigLenRight = framesPerBuffer + data->irRight_len - 1;
	int maxLen = (convSigLenLeft > convSigLenRight) ? convSigLenLeft : convSigLenRight;

    //hold time domain audio and IR signals
    double *inLeft = fftw_malloc(sizeof(double) * maxLen );
    double *inRight = fftw_malloc(sizeof(double) * maxLen );
    double *inIRLeft = fftw_malloc(sizeof(double) * maxLen );
    double *inIRRight = fftw_malloc(sizeof(double) * maxLen );
    double *convolvedSigLeft;
    double *convolvedSigRight;


	//Store samples into in and inIR buffers
    for (i = 0; i < maxLen; i++)
    {
        inLeft[i] = (i < framesPerBuffer) ? data->file_buff[i]: 0;
        inRight[i] = (i < framesPerBuffer) ? data->file_buff[i]: 0;

        inIRLeft[i] = (i < data->irLeft_len) ? data->irLeftBuffer[i] : 0;
        inIRRight[i] = (i < data->irRight_len) ? data->irRightBuffer[i] : 0;
    }
    
    // Convolve signals via convolution function
    convolve(inLeft, framesPerBuffer, inIRLeft, data->irLeft_len, &convolvedSigLeft);
    convolve(inRight, framesPerBuffer, inIRRight, data->irRight_len, &convolvedSigRight);
    
    /********************************************
     **** COPY DATA INTO APPROPRIATE BUFFERS ****
    *********************************************/
    for (i = 0; i < maxLen; i++)
    {  
       //Normalized convolved signals
       convolvedSigLeft[i] = (i < convSigLenLeft) ? (convolvedSigLeft[i] /(double)convSigLenLeft) : 0;
       convolvedSigRight[i] = (i < convSigLenRight) ? (convolvedSigRight[i] /(double)convSigLenRight) : 0;

       RESULTBUFFER_LEFT[(resultIxLeft) % BUFFERSIZE_MAX] += convolvedSigLeft[i];
       RESULTBUFFER_RIGHT[(resultIxRight) % BUFFERSIZE_MAX] += convolvedSigRight[i];
       
       if (i < framesPerBuffer)
       {
		   //store result buffer values into output
	       out[2*i] = (float) RESULTBUFFER_LEFT[resultIxLeft % BUFFERSIZE_MAX];
	       out[2*i + 1] = (float) RESULTBUFFER_RIGHT[resultIxRight % BUFFERSIZE_MAX];
	       
	       //zero out the result buffer value so that the index can be used again the next loop around
	       RESULTBUFFER_LEFT[resultIxLeft % BUFFERSIZE_MAX] = 0;
	       RESULTBUFFER_RIGHT[resultIxRight % BUFFERSIZE_MAX] = 0;
       }
       
       resultIxLeft++;
       resultIxRight++;
    }
    
	resultIxLeft -= (convSigLenLeft - framesPerBuffer);
	resultIxRight -= (convSigLenRight - framesPerBuffer);
		
	/**** Free up allocated memory *******/

	//Free memory for all buffers
    fftw_free(inLeft);
    fftw_free(inRight);
    fftw_free(inIRLeft);
    fftw_free(inIRRight);
   	fftw_free(convolvedSigLeft);
    fftw_free(convolvedSigRight);

    //update indices
    if (endOfIR) {
        irIx = 0;
        endOfIR = false;
    }
    else {
        irIx += framesPerBuffer;
    }

    return paContinue;
}

/*********************************************** 
 *********************************************** 
 ***************  MAIN FUNCTION ****************
 ***********************************************
************************************************/

int main( int argc, char **argv ) {

    PaStream *stream;
    PaStreamParameters outputParameters;
    PaStreamParameters inputParameters;
    PaError err;
    paData data;

    SF_INFO leftIRdata, rightIRdata;

    char *leftIRfilename, *rightIRfilename, *audioFilename; // *impulseFilename


    /* Check arguments */
    if ( argc != 4 ) {
        printf("Usage: %s audio_filename.wav left_HRTF_filename.wav right_HRTF_filename.wav\n", argv[0]);
        return EXIT_FAILURE;
    }

    audioFilename = argv[1];
    leftIRfilename = argv[2];
    rightIRfilename = argv[3];
    
    //Open audio file to play
	initAudioData(audioFilename, &data, 0, 0);

	// Initialize pair of impulse responses
    initIRPair(leftIRfilename, &leftIRdata, rightIRfilename, &rightIRdata, &data);
	

    /*************************************
    ** Initiate Port Audio data values ***
    **************************************/
    

	// Scaling factors
    data.ampScal = .5;
    data.ampScal2 = .1;

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
    int row, col;
    
    /* Initialize interactive character input */
	initscr(); // Start curses mode
    keypad(stdscr, TRUE);

    char ch;
    ch = '\0'; /* Init ch to null character */
	getmaxyx(stdscr,row,col);
      
    //Print info about audio and impulse response
    mvprintw(0, col/3, "%s%d", "Buffer Size: ", FRAMES_PER_BUFFER);
    
    mvprintw(7, 0, "%s%s", "Left Impulse Response: ", leftIRfilename);
    mvprintw(8, 0, "%s%d", "Number of channels: ", leftIRdata.channels);
    mvprintw(9, 0, "%s%d", "Number of frames: ", leftIRdata.frames);
    
    mvprintw(7, col/2, "%s%s", "Right Impulse Response: ", rightIRfilename);
    mvprintw(8, col/2, "%s%d", "Number of channels: ", rightIRdata.channels);
    mvprintw(9, col/2, "%s%d", "Number of frames: ", rightIRdata.frames);

	mvprintw(2, 0, "%s%s", "Audio Signal: ", audioFilename);
	mvprintw(3, 0, "%s%d", "Number of channels: ", data.sf_inInfo.channels);
	mvprintw(4, 0, "%s%d", "Number of frames: ", data.sf_inInfo.frames);
	
    mvprintw(row/2 - 5, 0, "%s", "Type 'q' to quit: ");
	
	refresh();

    while (ch != 'q') {

        switch (ch)
        {
        	case 65:
        		if ( (data.elevation + ELE_INC) <= ELE_MAX)
	        		data.elevation = (data.elevation + 5);
        		break;
        	case 66:
	        	if ( (data.elevation - ELE_INC) >= ELE_MIN)
		        	data.elevation = (data.elevation - 5);
        		break;
        	case 67:
	        	data.azimuth = (data.azimuth + 360 + 5) % 360;
        		break;
        	case 68:
	        	data.azimuth = (data.azimuth + 360 - 5) % 360;
        		break;	
        }
        mvprintw(row/2, 0, "%s%5d", "Azimuth: ", data.azimuth);
        mvprintw(row/2 + 1, 0, "%s%5d", "Elevation: ", data.elevation);
        mvprintw(row/2 + 2, 0, "%s%c", "Typed character: ", ch);
        //mvprintw(row/2 + 3, 0, "%s%d", "Character length?...", strlen(&ch));
        refresh();
        ch = getchar();
    }
    
    /* End curses mode  */
    endwin();
    

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
 ****** INITIALIZE AUDIO SOURCE DATA ***********
 ***********************************************
 ***********************************************/

void initAudioData(char *filename, paData *data, int az, int elev)
{
    SNDFILE *audioFile;
    SF_INFO audioData;

    if ((audioFile = sf_open(filename, SFM_READ, &audioData)) == NULL ) 
    {
        printf("Error, could not open audio file%s.\n", filename);
        puts(sf_strerror(NULL));
        return;
    }
    
    data->sf_inInfo = audioData;
    data->infile = audioFile;
    
    data->azimuth = az;
    data->elevation = elev;
}

/*********************************************** 
 *********************************************** 
 ****** INITIALIZE IMPULSE RESPONSE DATA *******
 ***********************************************
 ***********************************************/

void initIRPair(char *leftFilename, SF_INFO *leftIR, char *rightFilename, SF_INFO *rightIR, paData *data)
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
	data->irLeft_len = irLeftLen;
	data->irRight_len = irRightLen;
}
