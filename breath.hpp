#include "fmod.hpp"
#include <stdlib.h>
#include <string.h>

/* DLL Generation */
extern "C" {
	F_EXPORT FMOD_DSP_DESCRIPTION* F_CALL FMODGetDSPDescription();
}

#define INTRF_NUM_PARAMETERS 3

FMOD_RESULT F_CALLBACK ReadCallback(FMOD_DSP_STATE* dsp_state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int* outchannels);
FMOD_RESULT F_CALLBACK CreateCallback(FMOD_DSP_STATE* dsp_state);
FMOD_RESULT F_CALLBACK ReleaseCallback(FMOD_DSP_STATE* dsp_state);
FMOD_RESULT F_CALLBACK SetParamDataCallback(FMOD_DSP_STATE* dsp_state, int index, void* value, unsigned int length);
FMOD_RESULT F_CALLBACK SetParamFloatCallback(FMOD_DSP_STATE* dsp_state, int index, float value);
FMOD_RESULT F_CALLBACK GetParamFloatCallback(FMOD_DSP_STATE* dsp_state, int index, float* value, char* valstr);

static FMOD_DSP_PARAMETER_DESC breath;
static FMOD_DSP_PARAMETER_DESC frequency;
static FMOD_DSP_PARAMETER_DESC distance;

typedef struct
{
	//Data storage
	float* buffer;
	float* processed_buffer;

	//Params
	float* breath; // Breath audio
	unsigned int breath_index; // index to read breath audio
	unsigned int breath_samples; // length of the breath audio

	// TODO: Fill with an array of breaths (in, out, mid)
	float freq;	// Frequency of breathing mid
	float distance; //Distance between another breaths with mid

	float breath_mid_fade_time;
	float blank_window;
	float threshold;
	unsigned int* markers_out;
	unsigned int markers_out_index;
	unsigned int* markers_in;
	unsigned int markers_in_index;

	//Struct params
	int   length_samples;
	int   channels;
} dsp_data;

extern FMOD_DSP_PARAMETER_DESC* paramdesc[INTRF_NUM_PARAMETERS];

extern FMOD_DSP_DESCRIPTION FMOD_Breathalog_Desc;