#include "fmod.hpp"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <list>
#include <vector>
#include <time.h>
#include <algorithm>

/* DLL Generation */
extern "C" {
	F_EXPORT FMOD_DSP_DESCRIPTION* F_CALL FMODGetDSPDescription();
}

#define INTRF_NUM_PARAMETERS 5

FMOD_RESULT F_CALLBACK ReadCallback(FMOD_DSP_STATE* dsp_state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int* outchannels);
FMOD_RESULT F_CALLBACK CreateCallback(FMOD_DSP_STATE* dsp_state);
FMOD_RESULT F_CALLBACK ReleaseCallback(FMOD_DSP_STATE* dsp_state);
FMOD_RESULT F_CALLBACK SetParamDataCallback(FMOD_DSP_STATE* dsp_state, int index, void* value, unsigned int length);
FMOD_RESULT F_CALLBACK SetParamIntCallback(FMOD_DSP_STATE* dsp_state, int index, int value);
FMOD_RESULT F_CALLBACK GetParamIntCallback(FMOD_DSP_STATE* dsp_state, int index, int* value, char* valstr);

static FMOD_DSP_PARAMETER_DESC dialog;
static FMOD_DSP_PARAMETER_DESC breathInAudio;
static FMOD_DSP_PARAMETER_DESC breathOutAudio;
static FMOD_DSP_PARAMETER_DESC breathFreqAudio;
static FMOD_DSP_PARAMETER_DESC breathFrequency;

typedef struct dsp
{
	//Params
	float* dialog;							// Dialog audio
	std::vector<float*> breathInAudios;		// Breath in audios
	std::vector<float*> breathOutAudios;	// Breath out audios
	std::vector<float*> breathFreqAudios;	// Breath freq audios
	unsigned int breathFrequency;			// Breath freq frequency

	//Variables
	float* dialogThr;						// Buffer to store dialog thresholded
	float* breathInAudio;
	float* breathOutAudio;
	float* breathFreqAudio;
	unsigned int dialogIndex;				// Index to read dialog audio
	unsigned int dialogSamples;				// Length of the dialog audio
	unsigned int breathOutIndex;			// Index to point to a breath out audio
	unsigned int breathOutReadIndex;		// Index to read current breath out audio
	unsigned int breathInIndex;				// Index to point to a breath in audio
	unsigned int breathInReadIndex;			// Index to read current breath in audio
	unsigned int breathFreqIndex;			// Index to point to a breath freq audio
	unsigned int breathFreqReadIndex;		// Index to read current breath freq audio
	unsigned int breathFreqCountIndex;		// Index to count for a breath freq audio
	float dialogFindWindow;					// Window time to calculate real dialog
	float threshold;						// Threshold to calculate dialog volume
	bool breathingIn;						// To know if it's breathing in (priority)
	unsigned int breathFadeCounter;			// To control the fade between out and in breaths
} dsp_data;

unsigned int breathOutSamples;				// Breath out length in samples 
unsigned int breathInSamples;				// Breath in length in samples 
unsigned int breathFreqSamples;				// Breath freq length in samples 
std::list<unsigned int> markersOut;			// List of breath out markers
std::list<unsigned int> markersIn;			// List of breath in markers
std::list<unsigned int>::iterator markerOutIndex;// Index to track which marker out to search for
std::list<unsigned int>::iterator markerInIndex;	// Index to track which marker in to search for

extern FMOD_DSP_PARAMETER_DESC* paramdesc[INTRF_NUM_PARAMETERS];

extern FMOD_DSP_DESCRIPTION FMOD_Breathalog_Desc;