/*==========================================
Breathalog Plugin v0.1 for FMOD
Matias Lizana García 2019
===========================================*/

#define _CRT_SECURE_NO_WARNINGS
#include "breath.hpp"

/* DLL GENERATION */

FMOD_DSP_PARAMETER_DESC* paramdesc[] =
{
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
		FMOD_DSP_INIT_PARAMDESC_DATA(breath, "wave data", "", "wave data", FMOD_DSP_PARAMETER_DATA_TYPE_USER);
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
	
	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK ReadCallback(FMOD_DSP_STATE* dsp_state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int* outchannels)
{
	dsp_data* data = (dsp_data*)dsp_state->plugindata;

	for (unsigned int samp = 0; samp < length; samp++) {
		outbuffer[samp] = data->breath[data->breath_index];
		if (data->breath_index++ > data->breath_samples)
			data->breath_index = 0;
	}

	//TODO: Com buidar els marcadors cada cop que processem noves dades del buffer?
	/*
	//We assume its mono
	dsp_data* data = (dsp_data*)dsp_state->plugindata;

	//We store the thresholded audio, to know where to discriminate values for a dialog
	for (unsigned int samp = 0; samp < length; samp++)
		data->processed_buffer[samp] = abs(data->buffer[samp]) > data->threshold ? 1 : 0;

	//2. Find the markers for the dialog in/out using a window
	for (unsigned int samp = 1; samp < length; samp++) {
		bool found = 0;
		//Comes from dialog and now it's silence
		if (data->processed_buffer[samp] == 0 && data->processed_buffer[samp - 1] == 1) {
			//Uses the blank window to fill gaps that are still dialog
			for (unsigned int t = 0; t < data->blank_window; t++) {
				//If found some dialog, quit
				if (data->processed_buffer[samp + t] == 1)
					data->processed_buffer[samp] = 1;
				found = 1;
				break;
			}
			//If there's no dialog on this window, it's a mark out
			if (found == 0) {
				data->markers_out[data->markers_out_index] = samp;
				data->markers_out_index++;
			}
		}
		//Comes from silence and now it's dialog
		else if(data->processed_buffer[samp] == 1 && data->processed_buffer[samp-1] == 0) {

		}
	}
	*/
	   
	//outbuffer[(samp * inchannels) + chan] = inbuffer[(samp * inchannels) + chan] * voice_shatter + noise;

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
	if (index == 0)
	{
		dsp_data* data = (dsp_data*)dsp_state->plugindata;

		data->breath = (float*) malloc(length);
		if (!data->breath)
			return FMOD_ERR_MEMORY;

		data->breath = (float*) value;  
		data->breath_index = 0;	// Initialize index pointer
		data->breath_samples = length / 4;

		return FMOD_OK;
	}

	return FMOD_ERR_INVALID_PARAM;
}

FMOD_RESULT F_CALLBACK SetParamFloatCallback(FMOD_DSP_STATE* dsp_state, int index, float value)
{
	dsp_data* mydata = (dsp_data*)dsp_state->plugindata;
	/*
	if (index == 1)
		mydata->voice_shatter = value;
	else if (index == 2)
		mydata->noise_volume = value;
	else if (index == 3)
		mydata->noise_shatter = value;
	else
		return FMOD_ERR_INVALID_PARAM;
		*/
	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK GetParamFloatCallback(FMOD_DSP_STATE* dsp_state, int index, float* value, char* valstr)
{
	dsp_data* mydata = (dsp_data*)dsp_state->plugindata;

	/*
	if (index == 1)
		* value = mydata->voice_shatter;
	else if (index == 2)
		* value = mydata->noise_volume;
	else if (index == 3)
		* value = mydata->noise_shatter;
	else
		return FMOD_ERR_INVALID_PARAM;

		*/
	return FMOD_OK;
}