#include "fmod.hpp"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <list>

/* DLL Generation */
extern "C" {
	F_EXPORT FMOD_DSP_DESCRIPTION* F_CALL FMODGetDSPDescription();
}

#define INTRF_NUM_PARAMETERS 4

FMOD_RESULT F_CALLBACK ReadCallback(FMOD_DSP_STATE* dsp_state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int* outchannels);
FMOD_RESULT F_CALLBACK CreateCallback(FMOD_DSP_STATE* dsp_state);
FMOD_RESULT F_CALLBACK ReleaseCallback(FMOD_DSP_STATE* dsp_state);
FMOD_RESULT F_CALLBACK SetParamDataCallback(FMOD_DSP_STATE* dsp_state, int index, void* value, unsigned int length);
FMOD_RESULT F_CALLBACK SetParamFloatCallback(FMOD_DSP_STATE* dsp_state, int index, float value);
FMOD_RESULT F_CALLBACK GetParamFloatCallback(FMOD_DSP_STATE* dsp_state, int index, float* value, char* valstr);

void CalculatePreprocessBuffer();

static FMOD_DSP_PARAMETER_DESC dialog;
static FMOD_DSP_PARAMETER_DESC breath;
static FMOD_DSP_PARAMETER_DESC frequency;
static FMOD_DSP_PARAMETER_DESC distance;

typedef struct dsp
{
	//Params
	float* dialog;							// Dialog audio
	float* breath;							// Breath audio TODO: Fill with an array of breaths (in, out, mid)
	float freq;								// Frequency of breathing mid
	float distance;							// Distance between another breaths with mid

	//Variables
	float* dialog_thr;						// Buffer to store dialog thresholded
	unsigned int dialog_index;				// Index to read dialog audio
	unsigned int dialog_samples;			// Length of the dialog audio
	unsigned int breath_index;				// Index to read breath audio
	unsigned int breath_samples;			// Length of the breath audio
	float breath_mid_fade_time;				// Fade time to mix dialog with breath
	float dialog_find_window;				// Window time to calculate real dialog
	float threshold;						// Threshold to calculate dialog volume
} dsp_data;

std::list<unsigned int> markers_out;			// List of breath out markers
std::list<unsigned int> markers_in;				// List of breath in markers
std::list<unsigned int>::iterator mout_index;	// Index to track which marker out to search for
std::list<unsigned int>::iterator min_index;	// Index to track which marker in to search for

extern FMOD_DSP_PARAMETER_DESC* paramdesc[INTRF_NUM_PARAMETERS];

extern FMOD_DSP_DESCRIPTION FMOD_Breathalog_Desc;