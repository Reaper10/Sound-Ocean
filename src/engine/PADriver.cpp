/*
    Copyright (C) 2007-2010 Remon Sijrier
 
    This file is part of Traverso
 
    Traverso is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 
*/

#include "PADriver.h"

#include "AudioDevice.h"
#include "AudioChannel.h"

#include <Utils.h>

// Always put me below _all_ includes, this is needed
// in case we run with memory leak detection enabled!
#include "Debugger.h"


// TODO Is there an xrun callback for PortAudio? If so, connect to _xrun_callback
// TODO If there is some portaudio shutdown callback, connect to _on_pa_shutdown_callback
//	and make it work!

PADriver::PADriver( AudioDevice * dev , int rate, nframes_t bufferSize)
        : TAudioDriver(dev, rate, bufferSize)
{
	read = MakeDelegate(this, &PADriver::_read);
	write = MakeDelegate(this, &PADriver::_write);
	run_cycle = RunCycleCallback(this, &PADriver::_run_cycle);
}

PADriver::~PADriver( )
{
	PENTER;

}

int PADriver::_read(nframes_t nframes)
{
        float* in = (float*) m_paInputBuffer;
	
        if (!m_captureChannels.size()) {
		return 0;
	}

        int index = 0;
        for(nframes_t i=0; i<nframes; i++) {
                for (int chan=0; chan<m_captureChannels.size(); chan++) {
                        AudioChannel* channel = m_captureChannels.at(chan);
                        audio_sample_t* buf = channel->get_buffer(nframes);
                        buf[i] = in[index++];
                }
        }

	return 1;
}

int PADriver::_write(nframes_t nframes)
{
        if (!m_playbackChannels.size()) {
		return 0;
	}
	
        float* out = (float*) m_paOutputBuffer;


        int index = 0;
        for(nframes_t i=0; i<nframes; i++) {
                for (int chan=0; chan<m_playbackChannels.size(); chan++) {
                        out[index++] = m_playbackChannels.at(chan)->get_buffer(nframes)[i];
                }
        }
	
        for (int chan=0; chan<m_playbackChannels.size(); chan++) {
                m_playbackChannels.at(chan)->silence_buffer(nframes);
        }
	
	return 1;
}

QStringList PADriver::device_names(const QString& hostApi)
{
        QStringList list;
        PaError err = Pa_Initialize();

        if( err != paNoError ) {
                return list;
        }

        PaDeviceIndex deviceIndex = device_index_for_host_api(hostApi);

        if (deviceIndex == -1) {
                Pa_Terminate();
                return list;
        }

        const PaHostApiInfo* hostApiInfo = Pa_GetHostApiInfo(deviceIndex);

        for (int i=0; i<hostApiInfo->deviceCount; ++i) {
                list.append(Pa_GetDeviceInfo(i)->name);
        }

        Pa_Terminate();

        return list;
}

PaDeviceIndex PADriver::device_index_for_host_api(const QString& hostapi)
{
        PaDeviceIndex deviceindex = paNoDevice;

        for (int i=0; i<Pa_GetHostApiCount(); ++i) {
                const PaHostApiInfo* inf = Pa_GetHostApiInfo(i);

                printf("PADriver::device_index_for_host_api: hostapi name is %s, deviceCount is %d\n", inf->name, inf->deviceCount);

                if (hostapi == "alsa" && inf->type == paALSA) {
                        printf("PADriver:: Found alsa host api\n");
                        deviceindex = i;
                        break;
                }

                if (hostapi == "jack" && inf->type == paJACK) {
                        printf("PADriver:: Found jack host api\n");
                        deviceindex = i;
                        break;
                }

                if (hostapi == "wmme" && inf->type == paMME) {
                        printf("PADriver:: Found wmme host api\n");
                        deviceindex = i;
                        break;
                }

                if (hostapi == "directsound" && inf->type == paDirectSound ) {
                        printf("PADriver:: Found directsound host api\n");
                        deviceindex = i;
                        break;
                }

                if (hostapi == "asio" && inf->type == paASIO ) {
                        printf("PADriver:: Found asio host api\n");
                        deviceindex = i;
                        break;
                }

                if (hostapi == "coreaudio" && inf->type == paCoreAudio ) {
                        printf("PADriver:: Found directsound host api\n");
                        deviceindex = i;
                        break;
                }
        }

        return deviceindex;
}

int PADriver::setup(bool capture, bool playback, const QString& hostapi)
{
	// TODO Only open the capture/playback stream if requested (capture == true, playback == true)
	
	// TODO use hostapi to detect which hostApi to use.
	// hostapi can be any of these:
	// Linux: alsa, jack, oss
	// Mac os x: coreaudio, jack
	// Windows: wmme, directx, asio
	
	// TODO In case of hostapi == "alsa", the callback thread prio needs to be set to realtime.
	// 	there has been some discussion on this on the pa mailinglist, digg it up!
	
        printf("PADriver:: capture, playback, hostapi: %d, %d, %s\n", capture, playback, QS_C(hostapi));
	
	PaError err = Pa_Initialize();
	
	if( err != paNoError ) {
		device->message(tr("PADriver:: PortAudio error: %1").arg(Pa_GetErrorText( err )), AudioDevice::WARNING);
		Pa_Terminate();
		return -1;
        }
	
	PaStreamParameters outputParameters, inputParameters;

        PaDeviceIndex deviceindex = device_index_for_host_api(hostapi);

        if (deviceindex == paNoDevice) {
                device->message(tr("PADriver:: hostapi %1 was not found by Portaudio, trying default device!").arg(hostapi), AudioDevice::WARNING);
                deviceindex = Pa_GetDefaultOutputDevice();
                if( deviceindex == paNoDevice) {
                        device->message(tr("PADriver:: No default output device either, can't setup an audio device.").arg(hostapi), AudioDevice::WARNING);
                        Pa_Terminate();
                        return -1;
                }
	}
	
//	device->message(tr("PADriver:: using device %1").arg(deviceindex), AudioDevice::INFO);
		
        int inChannelMax = 0;
        int outChannelsMax = 0;

	// Configure output parameters.
        if( deviceindex != paNoDevice) {
                outChannelsMax = Pa_GetDeviceInfo(deviceindex)->maxOutputChannels;
                outputParameters.device = deviceindex;
                outputParameters.channelCount = outChannelsMax;
		outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
		outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
		outputParameters.hostApiSpecificStreamInfo = NULL;
	}
	
        if( deviceindex != paNoDevice) {
                inChannelMax = Pa_GetDeviceInfo(deviceindex)->maxInputChannels;
                inputParameters.device = deviceindex;
                inputParameters.channelCount = inChannelMax;
		inputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
                inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
		inputParameters.hostApiSpecificStreamInfo = NULL;
	}
	

        /* Open an audio I/O stream. */
	err = Pa_OpenStream(
			&m_paStream,
			&inputParameters,	// The input parameter
			&outputParameters,	// The outputparameter
			frame_rate,		// Set in the constructor
			frames_per_cycle,	// Set in the constructor
			paNoFlag,		// Don't use any flags
			_process_callback, 	// our callback function
			this );
	
	if( err != paNoError ) {
		device->message(tr("PADriver:: PortAudio error: %1").arg(Pa_GetErrorText( err )), AudioDevice::WARNING);
		Pa_Terminate();
		return -1;
	} else {
		printf("PADriver:: Succesfully opened portaudio stream\n");
	}
	
        AudioChannel* audiochannel;
        char buf[32];

        for (int chn = 0; chn < outChannelsMax; chn++) {

                snprintf (buf, sizeof(buf) - 1, "playback_%d", chn+1);

                audiochannel = add_playback_channel(buf);
                audiochannel->set_latency(frames_per_cycle + capture_frame_latency);
        }

        for (int chn = 0; chn < inChannelMax; chn++) {

                snprintf (buf, sizeof(buf) - 1, "capture_%d", chn+1);

                audiochannel = add_capture_channel(buf);
                audiochannel->set_latency(frames_per_cycle + capture_frame_latency);
        }

        return 1;
}

int PADriver::attach()
{
	return 1;
}

int PADriver::start( )
{
	PENTER;
	
	PaError err = Pa_StartStream( m_paStream );
	
	if( err != paNoError ) {
		device->message((tr("PADriver:: PortAudio error: %1").arg(Pa_GetErrorText( err ))), AudioDevice::WARNING);
		Pa_Terminate();
		return -1;
	} else {
		printf("PADriver:: Succesfully started portaudio stream\n");
	}
	
	return 1;
}

int PADriver::stop( )
{
	PENTER;
	PaError err = Pa_CloseStream( m_paStream );
	
	if( err != paNoError ) {
		device->message((tr("PADriver:: PortAudio error: %1").arg(Pa_GetErrorText( err ))), AudioDevice::WARNING);
		Pa_Terminate();
	} else {
		printf("PADriver:: Succesfully closed portaudio stream\n\n");
	}
	
	return 1;
}

int PADriver::process_callback (nframes_t nframes)
{
	if (device->run_cycle( nframes, 0.0) == -1) {
		return paAbort;
	}
	
	return paContinue;
}

QString PADriver::get_device_name( )
{
	// TODO get it from portaudio ?
	return "AudioDevice";
}

QString PADriver::get_device_longname( )
{
	// TODO get it from portaudio ?
	return "AudioDevice";
}

int PADriver::_xrun_callback( void * arg )
{
	PADriver* driver  = static_cast<PADriver *> (arg);
	driver->device->xrun();
	return 0;
}

void PADriver::_on_pa_shutdown_callback(void * arg)
{
	Q_UNUSED(arg);
}

int PADriver::_process_callback(
	const void *inputBuffer,
	void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *arg )
{
	Q_UNUSED(timeInfo);
	Q_UNUSED(statusFlags);
	
	PADriver* driver  = static_cast<PADriver *> (arg);
	
        driver->m_paInputBuffer = (void*)inputBuffer;
        driver->m_paOutputBuffer = outputBuffer;
	
	driver->process_callback (framesPerBuffer);
	
	return 0;
}

float PADriver::get_cpu_load( )
{
	return Pa_GetStreamCpuLoad(m_paStream) * 100;
}


//eof
