# C-Convolution-Engine

About:

This is an in-progress real-time convolution engine written in C. The program's end goal is to do the following in real-time:

1. Take up to 16 separate sound sources as inputs
2. Convolve each sound source with the appropriate pair of HRTFs based on its azimuth and elevation parameters
3. Output the sum of the convolution results to the user in the proper left and right channels
4. Allow the user to adjust location parameters while the program is running. The system should appropriately update each 
audio source's corresponding HRTFs with little to no time lag, noise, or other artifacts

To Run the Code ('1-Source' branch only!!):

1. Open the terminal and navigate to the directory that contains the code and Makefile.
2. Ensure that you also have at least 3 MONO .wav files in this directory: (left HRTF, right HRTF, and audio to playback e.g. a song of your choice).
3. Type 'make' and press Enter to compile.
4. Type './convEngine <leftHRTFfilename.wav> <rightHRTFfilename.wav> <audioFile.wav>' and press Enter to run.

Updates:

March 24, 2015:
  - So far, the "1-Source" branch allows for input of a single sound source and a choice of left & right HRTFs. It convolves a 4 1/2 minute song file with 512-sample HRTFs with no time lag or artifacts using a 64-sample buffer size in the audio callback
  - Am current modifying the code in the "Multi-Source" branch to allow for multiple audio source inputs. With the restructing, the program sometimes runs but other times produces either a seg fault or memory allocation error. Currently investigating.
