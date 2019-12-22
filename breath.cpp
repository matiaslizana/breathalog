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
	&threshold,
	&dialogWindow,
	&breathFadeTime,
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
	SetParamIntCallback,
	0,
	SetParamDataCallback,
	GetParamFloatCallback,
	GetParamIntCallback,
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
		FMOD_DSP_INIT_PARAMDESC_FLOAT(threshold, "Threshold", "dB", "Threshold for volume dialog detection", 0, 1, 0.001f);
		FMOD_DSP_INIT_PARAMDESC_INT(dialogWindow, "DialogWindow", "ms", "Time to find the dialog change", 0, 24000, 24000, 0, 0);
		FMOD_DSP_INIT_PARAMDESC_INT(breathFadeTime, "BreathFadeTime", "ms", "Time to mix the breath and dialog with a fade", 0, 24000, 12000, 0, 0);
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

	data->breathing = false;
	data->dialogWindow = 24000;
	srand((unsigned int)time(0));

	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK ReadCallback(FMOD_DSP_STATE* dsp_state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int* outchannels)
{
	dsp_data* data = (dsp_data*)dsp_state->plugindata;

	for (unsigned int s = 0; s < length; s++) {

		// Mix Breath OUT
		if (data->dialogIndex >= *markerOutIndex && data->breathOutReadIndex == data->dialogWindow) {
			data->breathing = true;
			outbuffer[s] = (data->breathOut[data->breathOutIndex])[data->breathOutReadIndex++];
		}
		// Breath OUT Ends
		else if (data->breathOutReadIndex == data->dialogWindow) {
			data->breathing = false;
			// Check if it has reached the last marker
			if (data->dialogIndex < markersOut.back()) {
				markerOutIndex++;
				data->breathOutReadIndex = 0;
				// Selects another breath out sample
				data->breathOutIndex = rand() % data->breathOut.size() - 1;
				while (data->breathOutIndex > data->breathOut.size() - 1)
					data->breathOutIndex = rand() % data->breathOut.size() - 1;
			}
		}
		// Mix Breath IN
		if (data->dialogIndex >= *markerInIndex && data->breathInReadIndex < data->dialogWindow) {
			data->breathing = true;
			unsigned int min = std::min(data->breathInReadIndex, data->breathFadeTime);
			float fadeAmount = (float) min / data->breathFadeTime;
			outbuffer[s]+= fadeAmount * data->dialog[data->dialogIndex] + (1.0f - fadeAmount) * (data->breathIn[data->breathInIndex])[data->breathInReadIndex];
			data->breathInReadIndex++;
		}
		// Breath IN Ends
		else if (data->breathInReadIndex == data->dialogWindow) {
			data->breathing = false;
			// Check if it has reached the last marker
			if (data->dialogIndex < markersIn.back()) {
				markerInIndex++;
				data->breathInReadIndex = 0;
				// Selects another breath in sample
				data->breathInIndex = rand() % data->breathIn.size() - 1;
				while (data->breathInIndex > data->breathIn.size() - 1)
					data->breathInIndex = rand() % data->breathIn.size() - 1;
			}
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
		for (unsigned int s = 1; s < data->dialogSamples - data->dialogWindow; s++) {
			bool found = 0;
			//Comes from dialog and now it's silence
			if (data->dialogThr[s] == 0 && data->dialogThr[s - 1] == 1) {
				//Uses the blank window to fill gaps that are still dialog
				for (unsigned int t = 0; t < data->dialogWindow; t++) {
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
				int inPivot = s - data->dialogWindow;
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
		for (unsigned int i = 0; i < length / 4; i += data->dialogWindow) {
			data->breathIn.push_back(&data->breathInAudio[i]);
			for (unsigned int s = i; s < i + data->dialogWindow; s++)
			{

			}
			data->breathInLengths.push_back();

		}
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
		for (unsigned int i = 0; i < length / 4; i += data->dialogWindow)
			data->breathOut.push_back(&data->breathOutAudio[i]);
		data->breathOutReadIndex = 0;	// Initialize read index pointer
		data->breathOutIndex = 0;		// Initialize breath index pointer

		return FMOD_OK;

	}
	else if (index == 3) {
		// BREATH HIT
		for (unsigned int i = 0; i < length / 4; i += data->dialogWindow)
			data->breathHit.push_back(&((float*)value)[i]);
		data->breathHitReadIndex = 0;	// Initialize read index pointer
		data->breathHitIndex = 0;		// Initialize breath index pointer
		return FMOD_OK;
	}
	return FMOD_ERR_INVALID_PARAM;
}

FMOD_RESULT F_CALLBACK SetParamFloatCallback(FMOD_DSP_STATE* dsp_state, int index, float value)
{
	dsp_data* data = (dsp_data*)dsp_state->plugindata;
	if (index == 4) {
		data->threshold = value;
		return FMOD_OK;
	}
	return FMOD_ERR_INVALID_PARAM;
}

FMOD_RESULT F_CALLBACK GetParamFloatCallback(FMOD_DSP_STATE* dsp_state, int index, float* value, char* valstr)
{
	dsp_data* data = (dsp_data*)dsp_state->plugindata;
	if (index == 4) {
		*value = data->threshold;
		return FMOD_OK;
	}
	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK SetParamIntCallback(FMOD_DSP_STATE* dsp_state, int index, int value)
{
	dsp_data* data = (dsp_data*)dsp_state->plugindata;
	if (index == 5) {
		data->dialogWindow = value;
		return FMOD_OK;
	}else if (index == 6) {
		data->breathFadeTime = value;
		return FMOD_OK;
	}
	return FMOD_ERR_INVALID_PARAM;
}

FMOD_RESULT F_CALLBACK GetParamIntCallback(FMOD_DSP_STATE* dsp_state, int index, int* value, char* valstr)
{
	dsp_data* data = (dsp_data*)dsp_state->plugindata;
	if (index == 4) {
		*value = data->dialogWindow;
		return FMOD_OK;
	}else if (index == 6) {
		*value = data->breathFadeTime;
		return FMOD_OK;
	}
	return FMOD_OK;
}