/*==========================================
Breathalog Plugin v0.1 for FMOD
Matias Lizana García 2019
===========================================*/

#define _CRT_SECURE_NO_WARNINGS
#include "breath.hpp"
#include <time.h>       /* time */
#include <algorithm>

/* DLL GENERATION */

FMOD_DSP_PARAMETER_DESC* paramdesc[] =
{
	&dialog,
	&breathIn,
	&breathOut,
	&breathHit,
};

FMOD_DSP_DESCRIPTION FMOD_Breathalog_Desc =
{
	FMOD_PLUGIN_SDK_VERSION,
	"Breathalog",   // name
	0x00010000,     // plug-in version
	1,              // number of input buffers to process
	1,              // number of output buffers to process
	CreateCallback,
	ReleaseCallback,
	0,
	ReadCallback,
	0,
	0,
	INTRF_NUM_PARAMETERS,
	paramdesc,
	SetParamFloatCallback,
	0,
	0,
	SetParamDataCallback,
	GetParamFloatCallback,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};

extern "C"
{
	F_EXPORT FMOD_DSP_DESCRIPTION* F_CALL FMODGetDSPDescription()
	{
		FMOD_DSP_INIT_PARAMDESC_DATA(dialog, "Dialog", "", "Dialog file", FMOD_DSP_PARAMETER_DATA_TYPE_USER);
		FMOD_DSP_INIT_PARAMDESC_DATA(breathIn, "BreathIn", "", "Breath In file", FMOD_DSP_PARAMETER_DATA_TYPE_USER);
		FMOD_DSP_INIT_PARAMDESC_DATA(breathOut, "BreathOut", "", "Breath Out file", FMOD_DSP_PARAMETER_DATA_TYPE_USER);
		FMOD_DSP_INIT_PARAMDESC_DATA(breathHit, "BreathHit", "", "Breath Hit file", FMOD_DSP_PARAMETER_DATA_TYPE_USER);
		return &FMOD_Breathalog_Desc;
	}
}

/* CALLBACKS */

FMOD_RESULT F_CALLBACK CreateCallback(FMOD_DSP_STATE* dsp_state)
{
	dsp_data* data = (dsp_data*)calloc(sizeof(dsp_data), 1);
	if (!data)
		return FMOD_ERR_MEMORY;
	dsp_state->plugindata = data;

	data->threshold = 0.001f;
	data->dialogFindWindow = 15000;
	data->breathFadeTime = 10000;
	data->breathing = false;
	breathSamples = 24000;	//Breath length in samples (500 ms at 48000Khz)
	srand((unsigned int)time(0));

	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK ReadCallback(FMOD_DSP_STATE* dsp_state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int* outchannels)
{
	dsp_data* data = (dsp_data*)dsp_state->plugindata;

	for (unsigned int s = 0; s < length; s++) {

 		// Mix Breath OUT
		if (data->dialogIndex >= *markerOutIndex && data->breathOutReadIndex < breathSamples) {
			data->breathing = true;
			unsigned int min = std::min(data->breathOutReadIndex, data->breathFadeTime);
			float fadeAmount = (float) min / data->breathFadeTime;
			outbuffer[s] = (1.0f - fadeAmount) * data->dialog[data->dialogIndex] + fadeAmount * (data->breathOut[data->breathOutIndex])[data->breathOutReadIndex];
			data->breathOutReadIndex++;
		}
		else if (data->breathOutReadIndex == breathSamples && data->dialogIndex < markersOut.back()) {
			data->breathing = false;
			markerOutIndex++;
			data->breathOutReadIndex = 0;
			data->breathOutIndex = rand() % data->breathOut.size() - 1;
			while (data->breathOutIndex > data->breathOut.size() - 1)
				data->breathOutIndex = rand() % data->breathOut.size() - 1;
		}
		// Mix Breath IN
		if (!data->breathing && data->dialogIndex >= *markerInIndex && data->breathInReadIndex < breathSamples) {
			data->breathing = true;
			unsigned int min = std::min(data->breathInReadIndex, data->breathFadeTime);
			float fadeAmount = (float) min / data->breathFadeTime;
			outbuffer[s] = (1.0f - fadeAmount) * data->dialog[data->dialogIndex] + fadeAmount * (data->breathIn[data->breathInIndex])[data->breathInReadIndex];
			data->breathInReadIndex++;
		}
		else if (data->breathInReadIndex == breathSamples && data->dialogIndex < markersIn.back()) {
			data->breathing = false;
			markerInIndex++;
			data->breathInReadIndex = 0;
			data->breathInIndex = rand() % data->breathIn.size() - 1;
			while (data->breathInIndex > data->breathIn.size() - 1)
				data->breathInIndex = rand() % data->breathIn.size() - 1;
		}
		// Original Dialog
		if (!data->breathing)
			outbuffer[s] = data->dialog[data->dialogIndex];
		
		data->dialogIndex++;

		// Check if dialog has reached the end
		if (data->dialogIndex == data->dialogSamples) {
			data->dialogIndex = 0;
			data->breathOutIndex = 0;
			data->breathOutReadIndex = 0;
			data->breathInIndex = 0;
			data->breathInReadIndex = 0;
			markerInIndex = markersIn.begin();
			markerOutIndex = markersOut.begin();
		}
	}

	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK ReleaseCallback(FMOD_DSP_STATE* dsp_state)
{
	/*
	if (dsp_state->plugindata)
	{
		dsp_data* data = (dsp_data*)dsp_state->plugindata;

		if (data->buffer)
			free(data->buffer);

		if (data->processed_buffer)
			free(data->processed_buffer);

		if (data->markers_in)
			free(data->markers_in);

		if (data->markers_out)
			free(data->markers_out);

		free(data);
	}

	*/
	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK SetParamDataCallback(FMOD_DSP_STATE* dsp_state, int index, void* value, unsigned int length)
{
	dsp_data* data = (dsp_data*)dsp_state->plugindata;

	if (index == 0) {
		// DIALOG

		//0. Load dialog file
		data->dialog = (float*)malloc(length);
		if (!data->dialog)
			return FMOD_ERR_MEMORY;
		data->dialog = (float*)value;

		data->dialogIndex = 0;	// Initialize index pointer
		data->dialogSamples = length / 4; // Number of samples are floats

		data->dialogThr = (float*)malloc(length);
		if (!data->dialogThr)
			return FMOD_ERR_MEMORY;

		//1. We store the thresholded audio, to know where to discriminate values for a dialog
		for (unsigned int s = 0; s < data->dialogSamples; s++)
			data->dialogThr[s] = fabs(data->dialog[s]) > data->threshold ? 1.0f : 0.0f;

		//2. Find the markers for the dialog in/out using a window
		markersIn.clear();
		markersOut.clear();
		for (unsigned int s = 1; s < data->dialogSamples - data->dialogFindWindow; s++) {
			bool found = 0;
			//Comes from dialog and now it's silence
			if (data->dialogThr[s] == 0 && data->dialogThr[s - 1] == 1) {
				//Uses the blank window to fill gaps that are still dialog
				for (unsigned int t = 0; t < data->dialogFindWindow; t++) {
					//If found some dialog, quit
					if (data->dialogThr[s + t] == 1) {
						data->dialogThr[s] = 1;
						found = 1;
						break;
					}
				}
				//If there's no dialog on this window, it's a mark out
				if (found == 0)
					markersOut.push_back(s);
			}
			//Comes from silence and now it's dialog
			else if (data->dialogThr[s] == 1 && data->dialogThr[s - 1] == 0) {
				int inPivot = s - breathSamples;
				//Check if the breath fits at the begining of the audio
				if (inPivot >= 0)	//TODO: Check for overlapping in and out breaths
					markersIn.push_back(inPivot);
			}
		}

		markerInIndex = markersIn.begin();
		markerOutIndex = markersOut.begin();

		return FMOD_OK;

	}
	else if (index == 1) {
		// BREATH IN
		data->breathInAudio = (float*)malloc(length);
		if (!data->breathInAudio)
			return FMOD_ERR_MEMORY;
		memcpy(data->breathInAudio, value, length);
		for (unsigned int i = 0; i < length / 4; i += breathSamples)
			data->breathIn.push_back(&data->breathInAudio[i]);
		data->breathInReadIndex = 0;	// Initialize read index pointer
		data->breathInIndex = 0;		// Initialize breath index pointer

		return FMOD_OK;

	}
	else if (index == 2) {
		// BREATH OUT
		data->breathOutAudio = (float*)malloc(length);
		if (!data->breathOutAudio)
			return FMOD_ERR_MEMORY;
		memcpy(data->breathOutAudio, value, length);
		for (unsigned int i = 0; i < length / 4; i += breathSamples)
			data->breathOut.push_back(&data->breathOutAudio[i]);
		data->breathOutReadIndex = 0;	// Initialize read index pointer
		data->breathOutIndex = 0;		// Initialize breath index pointer

		return FMOD_OK;

	}
	else if (index == 3) {
		// BREATH HIT
		for (unsigned int i = 0; i < length / 4; i += breathSamples)
			data->breathHit.push_back(&((float*)value)[i]);
		data->breathHitReadIndex = 0;	// Initialize read index pointer
		data->breathHitIndex = 0;		// Initialize breath index pointer
		return FMOD_OK;
	}

	return FMOD_ERR_INVALID_PARAM;
}

FMOD_RESULT F_CALLBACK SetParamFloatCallback(FMOD_DSP_STATE* dsp_state, int index, float value)
{
	dsp_data* mydata = (dsp_data*)dsp_state->plugindata;
	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK GetParamFloatCallback(FMOD_DSP_STATE* dsp_state, int index, float* value, char* valstr)
{
	dsp_data* mydata = (dsp_data*)dsp_state->plugindata;
	return FMOD_OK;
}

void CalculatePreprocessBuffer() {

}