//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audiostream.cpp,v flo93 Exp $
//
//  (C) Copyright 2013 Florian Jung (florian.a.jung@web.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#include "audiostream.h"
#include <string>
using namespace std;

#ifdef RUBBERBAND_SUPPORT
	using namespace RubberBand;
#endif

namespace MusECore {

AudioStream::AudioStream(SndFileR sndfile_, int sampling_rate, int out_chans, bool stretching, XTick startXtick, unsigned startFrame)
{
	initalisation_failed = false; // not yet
	
	sndfile = sndfile_;
	
	output_sampling_rate=sampling_rate;
	n_output_channels=out_chans;
	doStretch=stretching;
#ifndef RUBBERBAND_SUPPORT
	if (doStretch) // stretching requested despite we have no support for this
	{
		printf("ERROR: AudioStream created with doStretch=true, but RUBBERBAND_SUPPORT not compiled in!\n");
		doStretch=false;
	}
#endif
	
	frameStartInSong=startFrame;
	xtickStartInSong=startXtick;
	
	n_input_channels = sndfile->channels();
	input_sampling_rate=sndfile->samplerate();
	
	if (!doStretch)
	{
#ifdef RUBBERBAND_SUPPORT
		stretcher=NULL; // not needed
#endif
		
		float src_ratio = (double)output_sampling_rate/input_sampling_rate;
		int error;
		srcState = src_new(SRC_SINC_MEDIUM_QUALITY, n_input_channels, &error); // TODO configure this
		if (!srcState) // panic!
		{
			printf("error creating new sample rate converter\n");
			initalisation_failed = true;
			return;
		}
		
		if (src_set_ratio(srcState, src_ratio))
		{
			printf("error setting sampling rate ratio\n");
			initalisation_failed = true;
			return;
		}
	}
	else
	{
#ifdef RUBBERBAND_SUPPORT
		srcState = NULL; // not needed
		
		stretcher = new RubberBandStretcher(sampling_rate, n_input_channels, 
					RubberBandStretcher::DefaultOptions, // TODO configure this
					1.0, 1.0); // these values will be overridden anyway soon.
		
		set_pitch_ratio(1.0); // this might call stretcher.setPitch() with something
		set_stretch_ratio(1.0); // different from 1.0, in order to do sampling rate conversion.
#endif
	}

	
	currentPositionInInput=0;
	currentPositionInOutput=0;
}

AudioStream::~AudioStream()
{
	if (srcState)
		src_delete(srcState);
	
#ifdef RUBBERBAND_SUPPORT
	if (stretcher)
		delete stretcher;
#endif
}

void AudioStream::seek(unsigned int frame, XTick xtick)
{
	unsigned int destFrame; // which frame in the input file to seek to.
	
	if (doStretch) // we're only interested in the xtick
		destFrame = xtickToFrameInFile(xtick);
	else // we're only interested in the frame
		destFrame = frame*input_sampling_rate/output_sampling_rate;
	
	sndfile->seek(destFrame, SEEK_SET);
	
	currentPositionInInput=destFrame; // that might be a lie if the stretcher still has stuff in behind? flush stretcher?
	currentPositionInOutput=frame;
	
	update_stretch_ratio();
}

#ifdef RUBBERBAND_SUPPORT
void AudioStream::set_stretch_ratio(double ratio)
{
	effective_stretch_ratio = ratio * output_sampling_rate / input_sampling_rate;
	stretcher->setTimeRatio(effective_stretch_ratio);
}

void AudioStream::set_pitch_ratio(double ratio)
{
	effective_pitch_ratio = ratio * input_sampling_rate / output_sampling_rate;
	stretcher->setPitchScale(effective_pitch_ratio);
}
#endif

unsigned int AudioStream::readAudio(float** deinterleaved_dest_buffer, int nFrames, bool overwrite)
{
	// convention: _buffers[] are interleaved, and deinterleaved_..._buffers[][] are deinterleaved
	
	/* there are two fundamentally different approaches: when time stretching using the rubberband library
	 * is done, then all sampling rate conversions are done by librubberband as well. given that the current
	 * stretch rate is 1.0, the input sampling rate is 22kHz and the output rate shall be 44kHz, then
	 * we tell rubberband to time-stretch by factor 0.5, and pitch-shift by the same factor. As a result,
	 * rubberband internally probably only pitchshifts, and we have the effect we want.
	 * 
	 * if no time stretching is wished, then we use libsamplerate for sample rate converting.
	 */
	
	if (doStretch)
	{
#ifdef RUBBERBAND_SUPPORT
		float* deinterleaved_result_buffer[n_input_channels];
		for (int i=0;i<n_input_channels;i++) deinterleaved_result_buffer[i]=new float[nFrames];
		
		size_t n_already_read = 0;
		while (n_already_read < nFrames)
		{
			size_t n_frames_to_read = stretcher->getSamplesRequired();
			float sndfile_buffer[n_frames_to_read*n_input_channels];
			float* deinterleaved_sndfile_buffer[n_input_channels];
			for (int i=0;i<n_input_channels;i++) deinterleaved_sndfile_buffer[i]=new float[n_frames_to_read];
			
			sndfile->readDirect(sndfile_buffer, n_frames_to_read);
			deinterleave(n_input_channels, sndfile_buffer, deinterleaved_sndfile_buffer, n_frames_to_read);
			currentPositionInInput+=n_frames_to_read;
			
			stretcher->process(deinterleaved_sndfile_buffer, n_frames_to_read, /*bool final = */false); // TODO: set final correctly!
			size_t n_frames_retrieved;
			
			n_frames_retrieved = stretcher->retrieve(deinterleaved_result_buffer+n_already_read, nFrames-n_already_read);
			
			for (int i=0;i<n_input_channels;i++) delete[] deinterleaved_sndfile_buffer[i];
			
			n_already_read += n_frames_retrieved;
		}
		
		copy_and_adjust_channels(n_input_channels, n_output_channels, deinterleaved_result_buffer, deinterleaved_dest_buffer, nFrames, overwrite);
		
		for (int i=0;i<n_input_channels;i++) delete[] deinterleaved_result_buffer[i];
#else
		printf("FATAL: THIS CANNOT HAPPEN: doStretch is true but RUBBERBAND_SUPPORT not compiled in!\n");
#endif
		
	}
	else
	{
		int n_frames_to_read = nFrames * input_sampling_rate / output_sampling_rate;
		float sndfile_buffer[n_frames_to_read*n_input_channels];
		float result_buffer[nFrames*n_input_channels];
		
		sndfile->readDirect(sndfile_buffer, n_frames_to_read);
		currentPositionInInput+=n_frames_to_read;

		SRC_DATA src_data;
		src_data.input_frames=n_frames_to_read;
		src_data.output_frames=nFrames;
		src_data.src_ratio = (double)output_sampling_rate/input_sampling_rate;
		src_data.data_in = sndfile_buffer;
		src_data.data_out = result_buffer;
		src_process(srcState, &src_data);
		
		if (src_data.input_frames_used != src_data.input_frames) // TODO
			printf("THIS IS AN ERROR (which was thought to not happen): src_data.input_frames_used = %li != src_data.input_frames = %li!\n",  src_data.input_frames_used , src_data.input_frames);
		
		int n_frames_read = src_data.output_frames_gen;
		while(n_frames_read < nFrames)
		{
			sndfile->readDirect(sndfile_buffer, 1);
			currentPositionInInput++;
			
			src_data.input_frames=1;
			src_data.output_frames=nFrames-n_frames_read;
			src_data.data_in=sndfile_buffer;
			src_data.data_out=result_buffer+n_frames_read;
			src_process(srcState, &src_data);

			if (src_data.input_frames_used != src_data.input_frames)
				printf("THIS IS AN ERROR (which was thought to not happen): src_data.input_frames_used = %li != src_data.input_frames = %li!\n",  src_data.input_frames_used , src_data.input_frames);
			
			n_frames_read+=src_data.output_frames_gen;
		}
		
		deinterleave_and_adjust_channels(n_input_channels, n_output_channels, result_buffer, deinterleaved_dest_buffer, nFrames, overwrite);
	}
	
	currentPositionInOutput+=nFrames;
	maybe_update_stretch_ratio();
	
	return nFrames;
}


void AudioStream::maybe_update_stretch_ratio()
{
	// TODO
	update_stretch_ratio();
}
void AudioStream::update_stretch_ratio()
{
#ifdef RUBBERBAND_SUPPORT
	if (doStretch)
	{
		XTick keyframe_xtick = frameToXTick(currentPositionInOutput) + XTick(386);
		
		unsigned keyframe_pos_in_stream = xtickToFrame(keyframe_xtick);
		unsigned keyframe_pos_in_file = xtickToFrameInFile(keyframe_xtick);
		
		// so we must play the file's frames from currentPositionInInput to keyframe_pos_in_file
		// within (keyframe_pos_in_stream-currentPositionInOutput) frames.
		
		double stretch_ratio = (double)(keyframe_pos_in_stream-currentPositionInOutput)/(keyframe_pos_in_file-currentPositionInInput);
		set_stretch_ratio(stretch_ratio);
	}
#endif
}

// converts the given frame-position of the output stream into the given XTick of the input stream
XTick AudioStream::frameToXTick(unsigned frame)
{
	return MusEGlobal::tempomap.frame2xtick(frame + frameStartInSong) - xtickStartInSong;
	//return externalTempoMap.frame2xtick(frame);
}

unsigned AudioStream::xtickToFrame(XTick xtick)
{
	unsigned retval = MusEGlobal::tempomap.tick2frame(xtick + xtickStartInSong);
	if (retval >= frameStartInSong) return retval-frameStartInSong;
	else
	{
		printf("ERROR: THIS SHOULD NEVER HAPPEN: AudioStream::xtickToFrame(XTick xtick) called with invalid xtick\n");
		return 0;
	}
	//return MusEGlobal::tempomap.tick2frame(xtick)-frameStartInSong;
	//return externalTempoMap.tick2frame(xtick);
}

unsigned AudioStream::xtickToFrameInFile(XTick xtick)
{
	return fileTempoMap.tick2frame(xtick);
}


} // namespace MusECore