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
	&breath,
	&frequency,
	&distance,
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
		FMOD_DSP_INIT_PARAMDESC_DATA(breath, "Breath", "", "Breath file", FMOD_DSP_PARAMETER_DATA_TYPE_USER);
		FMOD_DSP_INIT_PARAMDESC_FLOAT(frequency, "Frequency", "s", "frequency", 0, 2, 0);
		FMOD_DSP_INIT_PARAMDESC_FLOAT(distance, "Distance", "s", "distance", 0, 2, 0);
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
	data->dialog_find_window = 10000;

	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK ReadCallback(FMOD_DSP_STATE* dsp_state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int* outchannels)
{
	dsp_data* data = (dsp_data*)dsp_state->plugindata;

	for (unsigned int s = 0; s < length; s++) {
		// Dialog output
		outbuffer[s] = data->dialog[data->dialog_index];
		// Check if there's some breath out to mix
		if (data->dialog_index >= *mout_index && data->breath_out_index < breath_samples) {
			outbuffer[s] = data->breath[data->breath_out_index];
			data->breath_out_index++;
		// Breath is over
		} else if (data->breath_out_index == breath_samples && data->dialog_index < markers_out.back()) {
			mout_index++;
			data->breath_out_index = 0;
		}
		// Check if there's some breath in to mix
		if (data->dialog_index >= *min_index && data->breath_in_index < breath_samples) {
			outbuffer[s] = data->dialog[data->dialog_index] + data->breath[data->breath_in_index];
			data->breath_in_index++;
		// Breath is over
		} else if (data->breath_in_index == breath_samples && data->dialog_index < markers_in.back()) {
			min_index++;
			data->breath_in_index = 0;
		}
		// Check if dialog has reached the end
		if (data->dialog_index++ == data->dialog_samples) {
			data->dialog_index = 0;
			data->breath_out_index = 0;
			min_index = markers_in.begin();
			mout_index = markers_out.begin();
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

		//0. Load dialog file
		data->dialog = (float*)malloc(length);
		if (!data->dialog)
			return FMOD_ERR_MEMORY;
		data->dialog = (float*)value;
		data->dialog_index = 0;	// Initialize index pointer
		data->dialog_samples = length / 4; // Number of samples are floats

		data->dialog_thr = (float*)malloc(length);
		if (!data->dialog_thr)
			return FMOD_ERR_MEMORY;

		//1. We store the thresholded audio, to know where to discriminate values for a dialog
		for (unsigned int s = 0; s < data->dialog_samples; s++)
			data->dialog_thr[s] = fabs(data->dialog[s]) > data->threshold ? 1 : 0;

		//2. Find the markers for the dialog in/out using a window
		markers_in.clear();
		markers_out.clear();
		for (unsigned int s = 1; s < data->dialog_samples - data->dialog_find_window; s++) {
			bool found = 0;
			//Comes from dialog and now it's silence
			if (data->dialog_thr[s] == 0 && data->dialog_thr[s - 1] == 1) {
				//Uses the blank window to fill gaps that are still dialog
				for (unsigned int t = 0; t < data->dialog_find_window; t++) {
					//If found some dialog, quit
					if (data->dialog_thr[s + t] == 1) {
						data->dialog_thr[s] = 1;
						found = 1;
						break;
					}
				}
				//If there's no dialog on this window, it's a mark out
				if (found == 0)
					markers_out.push_back(s);
			}
			//Comes from silence and now it's dialog
			else if (data->dialog_thr[s] == 1 && data->dialog_thr[s - 1] == 0) {
				int inPivot = s - breath_samples;
				//Check if the breath fits at the begining of the audio
				if (inPivot >= 0)	//TODO: Check for overlapping in and out breaths
					markers_in.push_back(inPivot);
			}
		}

		min_index = markers_in.begin();
		mout_index = markers_out.begin();

		return FMOD_OK;
	}

	// TODO: Waiting to load more types of breath
	if (index == 1)
	{
		data->breath = (float*)malloc(length);
		if (!data->breath)
			return FMOD_ERR_MEMORY;

		data->breath = (float*)value;
		data->breath_out_index = 0;	// Initialize index pointer
		data->breath_in_index = 0;	// Initialize index pointer
		breath_samples = length / 4; // Number of samples are floats

		return FMOD_OK;
	}

	return FMOD_ERR_INVALID_PARAM;
}

FMOD_RESULT F_CALLBACK SetParamFloatCallback(FMOD_DSP_STATE* dsp_state, int index, float value)
{
	dsp_data* mydata = (dsp_data*)dsp_state->plugindata;

	if (index == 2)
		mydata->freq = value;

	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK GetParamFloatCallback(FMOD_DSP_STATE* dsp_state, int index, float* value, char* valstr)
{
	dsp_data* mydata = (dsp_data*)dsp_state->plugindata;

	if (index == 2)
		* value = mydata->freq;

	return FMOD_OK;
}

void CalculatePreprocessBuffer() {

}