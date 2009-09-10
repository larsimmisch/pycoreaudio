#include <CoreServices/CoreServices.h>
#include <stdio.h>
#include <unistd.h>
#include <AudioUnit/AudioUnit.h>
#include <stdio.h>
#include <pthread.h>
#include <sysexits.h>

typedef struct
{
	AudioUnit au;
	pthread_cond_t condition;
	pthread_mutex_t mutex;
	FILE *file;
	int running;
} Playback;

AudioUnit au_open_default()
{
    ComponentDescription cd;
	ComponentInstance *ci;
	Component c;
	AudioUnit au;
	int rc;

	cd.componentManufacturer = kAudioUnitManufacturer_Apple;
	cd.componentFlags = 0;
	cd.componentFlagsMask = 0;
	cd.componentType = kAudioUnitType_Output;
	cd.componentSubType = kAudioUnitSubType_DefaultOutput;
	
	c = FindNextComponent(NULL, &cd);
	rc = OpenAComponent(c, &au);
	if (rc)
	{
		printf("OpenAComponent failed: %d\n", rc);
		exit(EX_OSERR);
	}
    
	rc = AudioUnitInitialize(au);
	if (rc)
	{
		printf("AudioUnitInitialize failed: %d\n", rc);
		exit(EX_OSERR);
	}

	return au;
}

void au_prepare_mulaw(AudioUnit au)
{
	AudioStreamBasicDescription sd;
	int rc;

	/* 
	   Terminology from http://devworld.apple.com/documentation/MusicAudio/Reference/CAFSpec/CAF_overview/CAF_overview.html

	   Sample:
	   One number for one channel of digitized audio data.

	   Frame:
	   A set of samples representing one sample for each
	   channel. The samples in a frame are intended to be played
	   together (that is, simultaneously). Note that this definition
	   might be different from the use of the term "frame" by codecs,
	   video files, and audio or video processing applications.
	   
	   Packet: 
	   The smallest, indivisible block of data. For linear PCM
	   (pulse-code modulated) data, each packet contains exactly one
	   frame. For compressed audio data formats, the number of frames
	   in a packet depends on the encoding. For example, a packet of
	   AAC represents 1024 frames of PCM. In some formats, the number
	   of frames per packet varies.
	*/

	sd.mSampleRate = 8000;
	sd.mFormatID = kAudioFormatULaw;
	sd.mFormatFlags = kAudioFormatFlagIsNonInterleaved;
	sd.mBytesPerPacket = 1; // Bytes per frame * nchannels 
	sd.mFramesPerPacket = 1;
	sd.mBytesPerFrame = 1;
	sd.mChannelsPerFrame = 1;
	sd.mBitsPerChannel = 8;

	rc = AudioUnitSetProperty(au, 
							  kAudioUnitProperty_StreamFormat,
							  kAudioUnitScope_Input,
							  0,
							  &sd,
							  sizeof(AudioStreamBasicDescription));

	if (rc)
	{
		printf("AudioUnitSetProperty(StreamFormat) "
			   "failed: %d\n", rc);
		exit(EX_OSERR);
	}
}

/* This will be called in the context of a thread created by coreaudio
 */
OSStatus audio_unit_render_callback(
	void *inRefCon, 
	AudioUnitRenderActionFlags *ioActionFlags, 
	const AudioTimeStamp *inTimeStamp, 
	UInt32 inBusNumber, 
	UInt32 inNumberFrames, 
	AudioBufferList *ioData)
{
	Playback *pb = (Playback*)inRefCon;

	int rc = fread(ioData->mBuffers[0].mData, 
				   1, ioData->mBuffers[0].mDataByteSize, pb->file);

	if (rc <= 0)
	{
		pthread_mutex_lock(&pb->mutex);
		pb->running = 0;
		pthread_mutex_unlock(&pb->mutex);
		
		AudioOutputUnitStop(pb->au);
		pthread_cond_signal(&pb->condition);
	}

	return 0;
}

void au_set_callback(AudioUnit au, Playback *data)
{
    AURenderCallbackStruct input;
	int rc;

	input.inputProc = audio_unit_render_callback;
	input.inputProcRefCon = data;

	rc = AudioUnitSetProperty (au, 
							   kAudioUnitProperty_SetRenderCallback, 
							   kAudioUnitScope_Input,
							   0, 
							   &input, 
							   sizeof(input));

	if (rc)
	{
		printf("AudioUnitSetProperty(SetRenderCallback) "
			   "failed: %d\n", rc);
		exit(EX_OSERR);
	}
}

int main(int argc, char *argv[])
{
	int rc;
	Playback playback;

	if (argc < 2)
	{
		printf("usage: caplaymu <ulaw file>\n");
		exit(EX_USAGE);
	}

	playback.file = fopen(argv[1], "rb");
	if (!playback.file)
	{
		perror("Cannot open file");
		exit(EX_OSFILE);
	}

	pthread_mutex_init(&playback.mutex, NULL);
	pthread_cond_init(&playback.condition, NULL);

	playback.au = au_open_default();

	au_prepare_mulaw(playback.au);
	au_set_callback(playback.au, &playback);

	playback.running = 1;

	AudioOutputUnitStart(playback.au);

	pthread_mutex_lock(&playback.mutex);
	while (playback.running)
	{
		pthread_cond_wait(&playback.condition, &playback.mutex);
	}
	pthread_mutex_unlock(&playback.mutex);

}
