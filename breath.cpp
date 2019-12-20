/*==========================================
Breathalog Plugin v0.1 for FMOD
Matias Lizana García 2019
===========================================*/

#define _CRT_SECURE_NO_WARNINGS
#include "breath.hpp"

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

	data->threshold = 0.01f;
	data->dialogFindWindow = 10000;

	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK ReadCallback(FMOD_DSP_STATE* dsp_state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int* outchannels)
{
	dsp_data* data = (dsp_data*)dsp_state->plugindata;

	for (unsigned int s = 0; s < length; s++) {
		// Dialog output
		outbuffer[s] = data->dialog[data->dialogIndex];
		// Check if there's some breath out to mix
		if (data->dialogIndex >= *mOutIndex && data->breathOutIndex < breathSamples) {
			outbuffer[s] = data->breathOut[data->breathOutIndex];
			data->breathOutIndex++;
		// Breath is over
		} else if (data->breathOutIndex == breathSamples && data->dialogIndex < markersOut.back()) {
			mOutIndex++;
			data->breathOutIndex = 0;
		}
		// Check if there's some breath in to mix
		if (data->dialogIndex >= *mInIndex && data->breathInIndex < breathSamples) {
			outbuffer[s] = data->dialog[data->dialogIndex] + data->breathIn[data->breathInIndex];
			data->breathInIndex++;
		// Breath is over
		} else if (data->breathInIndex == breathSamples && data->dialogIndex < markersIn.back()) {
			mInIndex++;
			data->breathInIndex = 0;
		}
		// Check if dialog has reached the end
		if (data->dialogIndex++ == data->dialogSamples) {
			data->dialogIndex = 0;
			data->breathOutIndex = 0;
			mInIndex = markersIn.begin();
			mOutIndex = markersOut.begin();
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

	// DIALOG
	if (index == 0) {

		//0. Load dialog file
		data->dialog = (float*)value;
		data->dialogIndex = 0;	// Initialize index pointer
		data->dialogSamples = length / 4; // Number of samples are floats
		data->dialogThr = (float*)malloc(length);
		if (!data->dialogThr)
			return FMOD_ERR_MEMORY;

		//1. We store the thresholded audio, to know where to discriminate values for a dialog
		for (unsigned int s = 0; s < data->dialogSamples; s++)
			data->dialogThr[s] = fabs(data->dialog[s]) > data->threshold ? 1 : 0;

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

		mInIndex = markersIn.begin();
		mOutIndex = markersOut.begin();

		return FMOD_OK;
	}

	// BREATH IN
	if (index == 1)
	{
		data->breathIn = (float*)value;
		data->breathInIndex = 0;	// Initialize index pointer
		breathSamples = length / 4; // Number of samples are floats

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