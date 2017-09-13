/****************************************************************************
*                                                                           *
* Project64-audio - A Nintendo 64 audio plugin.                             *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2017 Project64. All rights reserved.                        *
* Copyright (C) 2015 Gilles Siberlin                                        *
* Copyright (C) 2007-2009 Richard Goedeken                                  *
* Copyright (C) 2007-2008 Ebenblues                                         *
* Copyright (C) 2003 JttL                                                   *
* Copyright (C) 2002 Hacktarux                                              *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "OpenSLES.h"
#include <Project64-audio/trace.h>
#include <Project64-audio/SettingsID.h>
#include <Project64-audio/AudioMain.h>

#ifdef ANDROID
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#endif

#ifdef ANDROID
typedef struct threadLock_
{
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    volatile unsigned char value;
    volatile unsigned char limit;
} threadLock;
#endif

/* number of bytes per sample */
enum
{
    N64_SAMPLE_BYTES = 4,
    SLES_SAMPLE_BYTES = 4,
};

/* Pointer to the primary audio buffer */
uint8_t * g_primaryBuffer = NULL;

/* Size of the primary buffer */
uint32_t g_primaryBufferBytes = 0;

/* Size of the primary audio buffer in equivalent output samples */
unsigned int g_PrimaryBufferSize = PRIMARY_BUFFER_SIZE;

/* Pointer to secondary buffers */
uint8_t ** g_secondaryBuffers = NULL;

/* Size of a single secondary buffer */
uint32_t g_secondaryBufferBytes = 0;

/* Size of a single secondary audio buffer in output samples */
uint32_t g_SecondaryBufferSize = SECONDARY_BUFFER_SIZE;

/* Position in the primary buffer where next audio chunk should be placed */
uint32_t g_primaryBufferPos = 0;

/* Index of the next secondary buffer available */
uint32_t g_secondaryBufferIndex = 0;

/* Number of secondary buffers */
uint32_t g_SecondaryBufferNbr = SECONDARY_BUFFER_NBR;

/* Audio frequency, this is usually obtained from the game, but for compatibility we set default value */
uint32_t g_GameFreq = DEFAULT_FREQUENCY;

/* SpeedFactor is used to increase/decrease game playback speed */
uint32_t g_speed_factor = 100;

/* If this is true then left and right channels are swapped */
bool g_SwapChannels = false;

/* Output Audio frequency */
int g_OutputFreq = 44100;

/* Indicate that the audio plugin failed to initialize, so the emulator can keep running without sound */
bool g_critical_failure = false;

#ifdef ANDROID
/* Thread Lock */
threadLock g_lock;

/* Engine interfaces */
SLObjectItf g_engineObject = NULL;
SLEngineItf g_engineEngine = NULL;

/* Output mix interfaces */
SLObjectItf g_outputMixObject = NULL;

/* Player interfaces */
SLObjectItf g_playerObject = NULL;
SLPlayItf g_playerPlay = NULL;

/* Buffer queue interfaces */
SLAndroidSimpleBufferQueueItf g_bufferQueue = NULL;
#endif

static bool CreatePrimaryBuffer(void)
{
    WriteTrace(TraceAudioInitShutdown, TraceDebug, "Start");
    unsigned int primaryBytes = (unsigned int)(g_PrimaryBufferSize * N64_SAMPLE_BYTES);

    WriteTrace(TraceAudioInitShutdown, TraceDebug, "Allocating memory for primary audio buffer: %i bytes.", primaryBytes);

    g_primaryBuffer = new uint8_t[primaryBytes];

    if (g_primaryBuffer == NULL)
    {
        WriteTrace(TraceAudioInitShutdown, TraceError, "g_primaryBuffer == NULL");
        WriteTrace(TraceAudioInitShutdown, TraceDebug, "Done (res: false)");
        return false;
    }

    memset(g_primaryBuffer, 0, primaryBytes);
    g_primaryBufferBytes = primaryBytes;
    WriteTrace(TraceAudioInitShutdown, TraceDebug, "Done (res: True)");
    return true;
}

static void CloseAudio(void)
{
    WriteTrace(TraceAudioInitShutdown, TraceDebug, "Start");
    g_primaryBufferPos = 0;
    g_secondaryBufferIndex = 0;

    /* Delete Primary buffer */
    if (g_primaryBuffer != NULL)
    {
        WriteTrace(TraceAudioInitShutdown, TraceDebug, "Delete g_primaryBuffer (%p)", g_primaryBuffer);
        g_primaryBufferBytes = 0;
        delete[] g_primaryBuffer;
        g_primaryBuffer = NULL;
    }

    /* Delete Secondary buffers */
    if (g_secondaryBuffers != NULL)
    {
        for (uint32_t i = 0; i < g_SecondaryBufferNbr; i++)
        {
            if (g_secondaryBuffers[i] != NULL)
            {
                WriteTrace(TraceAudioInitShutdown, TraceDebug, "Delete g_secondaryBuffers[%d] (%p)", i, g_secondaryBuffers[i]);
                delete[] g_secondaryBuffers[i];
                g_secondaryBuffers[i] = NULL;
            }
        }
        g_secondaryBufferBytes = 0;
        WriteTrace(TraceAudioInitShutdown, TraceDebug, "Delete g_secondaryBuffers (%p)", g_secondaryBuffers);
        delete[] g_secondaryBuffers;
        g_secondaryBuffers = NULL;
    }
#ifdef ANDROID
    /* Destroy buffer queue audio player object, and invalidate all associated interfaces */
    if (g_playerObject != NULL)
    {
        SLuint32 state = SL_PLAYSTATE_PLAYING;
        (*g_playerPlay)->SetPlayState(g_playerPlay, SL_PLAYSTATE_STOPPED);

        while (state != SL_PLAYSTATE_STOPPED)
        {
            (*g_playerPlay)->GetPlayState(g_playerPlay, &state);
        }

        (*g_playerObject)->Destroy(g_playerObject);
        g_playerObject = NULL;
        g_playerPlay = NULL;
        g_bufferQueue = NULL;
    }

    /* Destroy output mix object, and invalidate all associated interfaces */
    if (g_outputMixObject != NULL)
    {
        (*g_outputMixObject)->Destroy(g_outputMixObject);
        g_outputMixObject = NULL;
    }

    /* Destroy engine object, and invalidate all associated interfaces */
    if (g_engineObject != NULL)
    {
        (*g_engineObject)->Destroy(g_engineObject);
        g_engineObject = NULL;
        g_engineEngine = NULL;
    }

    /* Destroy thread Locks */
    pthread_cond_signal(&(g_lock.cond));
    pthread_mutex_unlock(&(g_lock.mutex));
    pthread_cond_destroy(&(g_lock.cond));
    pthread_mutex_destroy(&(g_lock.mutex));
#endif
    WriteTrace(TraceAudioInitShutdown, TraceDebug, "Done");
}

static bool CreateSecondaryBuffers(void)
{
    WriteTrace(TraceAudioInitShutdown, TraceDebug, "Start");
    bool status = true;
    unsigned int secondaryBytes = (unsigned int)(g_SecondaryBufferSize * SLES_SAMPLE_BYTES);

    WriteTrace(TraceAudioInitShutdown, TraceDebug, "Allocating memory for %d secondary audio buffers: %i bytes.", g_SecondaryBufferNbr, secondaryBytes);

    /* Allocate number of secondary buffers */
    g_secondaryBuffers = new uint8_t *[g_SecondaryBufferNbr];

    if (g_secondaryBuffers == NULL)
    {
        WriteTrace(TraceAudioInitShutdown, TraceError, "g_secondaryBuffers == NULL");
        WriteTrace(TraceAudioInitShutdown, TraceDebug, "Done (res: false)");
        return false;
    }

    /* Allocate size of each secondary buffers */
    for (uint32_t i = 0; i < g_SecondaryBufferNbr; i++)
    {
        g_secondaryBuffers[i] = new uint8_t[secondaryBytes];

        if (g_secondaryBuffers[i] == NULL)
        {
            status = false;
            break;
        }

        memset(g_secondaryBuffers[i], 0, secondaryBytes);
    }

    g_secondaryBufferBytes = secondaryBytes;
    WriteTrace(TraceAudioInitShutdown, TraceDebug, "Done (res: %s)", status ? "True" : "False");
    return status;
}

static int resample(unsigned char *input, int /*input_avail*/, int oldsamplerate, unsigned char *output, int output_needed, int newsamplerate)
{
    int *psrc = (int*)input;
    int *pdest = (int*)output;
    int i = 0, j = 0;

#ifdef USE_SPEEX
    spx_uint32_t in_len, out_len;
    if (Resample == RESAMPLER_SPEEX)
    {
        if (spx_state == NULL)
        {
            spx_state = speex_resampler_init(2, oldsamplerate, newsamplerate, ResampleQuality, &error);
            if (spx_state == NULL)
            {
                memset(output, 0, output_needed);
                return 0;
            }
        }
        speex_resampler_set_rate(spx_state, oldsamplerate, newsamplerate);
        in_len = input_avail / 4;
        out_len = output_needed / 4;

        if ((error = speex_resampler_process_interleaved_int(spx_state, (const spx_int16_t *)input, &in_len, (spx_int16_t *)output, &out_len)))
        {
            memset(output, 0, output_needed);
            return input_avail;  // number of bytes consumed
        }
        return in_len * 4;
    }
#endif
#ifdef USE_SRC
    if (Resample == RESAMPLER_SRC)
    {
        // the high quality resampler needs more input than the samplerate ratio would indicate to work properly
        if (input_avail > output_needed * 3 / 2)
            input_avail = output_needed * 3 / 2; // just to avoid too much short-float-short conversion time
        if (_src_len < input_avail * 2 && input_avail > 0)
        {
            if (_src) free(_src);
            _src_len = input_avail * 2;
            _src = malloc(_src_len);
        }
        if (_dest_len < output_needed * 2 && output_needed > 0)
        {
            if (_dest) free(_dest);
            _dest_len = output_needed * 2;
            _dest = malloc(_dest_len);
        }
        memset(_src, 0, _src_len);
        memset(_dest, 0, _dest_len);
        if (src_state == NULL)
        {
            src_state = src_new(ResampleQuality, 2, &error);
            if (src_state == NULL)
            {
                memset(output, 0, output_needed);
                return 0;
            }
        }
        src_short_to_float_array((short *)input, _src, input_avail / 2);
        src_data.end_of_input = 0;
        src_data.data_in = _src;
        src_data.input_frames = input_avail / 4;
        src_data.src_ratio = (float)newsamplerate / oldsamplerate;
        src_data.data_out = _dest;
        src_data.output_frames = output_needed / 4;
        if ((error = src_process(src_state, &src_data)))
        {
            memset(output, 0, output_needed);
            return input_avail;  // number of bytes consumed
        }
        src_float_to_short_array(_dest, (short *)output, output_needed / 2);
        return src_data.input_frames_used * 4;
    }
#endif
    // RESAMPLE == TRIVIAL
    if (newsamplerate >= oldsamplerate)
    {
        int sldf = oldsamplerate;
        int const2 = 2 * sldf;
        int dldf = newsamplerate;
        int const1 = const2 - 2 * dldf;
        int criteria = const2 - dldf;
        for (i = 0; i < output_needed / 4; i++)
        {
            pdest[i] = psrc[j];
            if (criteria >= 0)
            {
                ++j;
                criteria += const1;
            }
            else criteria += const2;
        }
        return j * 4; //number of bytes consumed
    }
    // newsamplerate < oldsamplerate, this only happens when speed_factor > 1
    for (i = 0; i < output_needed / 4; i++)
    {
        j = i * oldsamplerate / newsamplerate;
        pdest[i] = psrc[j];
    }
    return j * 4; //number of bytes consumed
}

/* This callback handler is called every time a buffer finishes playing */
#ifdef ANDROID
void queueCallback(SLAndroidSimpleBufferQueueItf caller, void *context)
{
    threadLock *plock = (threadLock *)context;

    pthread_mutex_lock(&(plock->mutex));

    if (plock->value < plock->limit)
        plock->value++;

    pthread_cond_signal(&(plock->cond));

    pthread_mutex_unlock(&(plock->mutex));
}
#endif

void OpenSLESDriver::AI_SetFrequency(uint32_t freq)
{
    WriteTrace(TraceAudioInitShutdown, TraceDebug, "Start (freq: %d)", freq);
    if (freq < 4000)
    {
        WriteTrace(TraceAudioInitShutdown, TraceInfo, "Sometimes a bad frequency is requested so ignore it (freq: %d)", freq);
        WriteTrace(TraceAudioInitShutdown, TraceDebug, "Done");
        return;
    }

    if (g_GameFreq == freq && g_primaryBuffer != NULL)
    {
        WriteTrace(TraceAudioInitShutdown, TraceInfo, "we are already using this frequency, so ignore it (freq: %d)", freq);
        WriteTrace(TraceAudioInitShutdown, TraceDebug, "Done");
        return;
    }

    if (g_critical_failure)
    {
        WriteTrace(TraceAudioInitShutdown, TraceInfo, "had a critical failure in setting up plugin, so ignore init");
        WriteTrace(TraceAudioInitShutdown, TraceDebug, "Done");
        return;
    }

    /* This is important for the sync */
    g_GameFreq = freq;

#ifdef ANDROID
    SLuint32 sample_rate;
    if ((freq / 1000) <= 11)
    {
        g_OutputFreq = 11025;
        sample_rate = SL_SAMPLINGRATE_11_025;
}
    else if ((freq / 1000) <= 22)
    {
        g_OutputFreq = 22050;
        sample_rate = SL_SAMPLINGRATE_22_05;
    }
    else if ((freq / 1000) <= 32)
    {
        g_OutputFreq = 32000;
        sample_rate = SL_SAMPLINGRATE_32;
    }
    else
    {
        g_OutputFreq = 44100;
        sample_rate = SL_SAMPLINGRATE_44_1;
    }
#endif

    WriteTrace(TraceAudioInitShutdown, TraceInfo, "Requesting frequency: %iHz.", g_OutputFreq);

    /* reload these because they gets re-assigned from data below, and InitializeAudio can be called more than once */
    g_PrimaryBufferSize = GetSetting(Buffer_PrimarySize);
    g_SecondaryBufferSize = GetSetting(Buffer_SecondarySize);
    g_SecondaryBufferNbr = GetSetting(Buffer_SecondaryNbr);

    /* Close everything because InitializeAudio can be called more than once */
    CloseAudio();

    /* Create primary buffer */
    if (!CreatePrimaryBuffer())
    {
        WriteTrace(TraceAudioInitShutdown, TraceError, "CreatePrimaryBuffer failed");
        CloseAudio();
        g_critical_failure = true;
        WriteTrace(TraceAudioInitShutdown, TraceDebug, "Done");
        return;
    }

    /* Create secondary buffers */
    if (!CreateSecondaryBuffers())
    {
        WriteTrace(TraceAudioInitShutdown, TraceError, "CreateSecondaryBuffers failed");
        CloseAudio();
        g_critical_failure = true;
        WriteTrace(TraceAudioInitShutdown, TraceDebug, "Done");
        return;
    }

#ifdef ANDROID
    /* Create thread Locks to ensure synchronization between callback and processing code */
    if (pthread_mutex_init(&(g_lock.mutex), (pthread_mutexattr_t*)NULL) != 0)
    {
        WriteTrace(TraceAudioInitShutdown, TraceError, "pthread_mutex_init failed");
        CloseAudio();
        g_critical_failure = true;
        WriteTrace(TraceAudioInitShutdown, TraceDebug, "Done");
        return;
    }
    if (pthread_cond_init(&(g_lock.cond), (pthread_condattr_t*)NULL) != 0)
    {
        WriteTrace(TraceAudioInitShutdown, TraceError, "pthread_cond_init failed");
        CloseAudio();
        g_critical_failure = true;
        WriteTrace(TraceAudioInitShutdown, TraceDebug, "Done");
        return;
    }
    pthread_mutex_lock(&(g_lock.mutex));
    g_lock.value = g_lock.limit = g_SecondaryBufferNbr;
    pthread_mutex_unlock(&(g_lock.mutex));

    /* Engine object */
    SLresult result = slCreateEngine(&g_engineObject, 0, NULL, 0, NULL, NULL);
    if (result != SL_RESULT_SUCCESS)
    {
        WriteTrace(TraceAudioInitShutdown, TraceError, "slCreateEngine failed (result: %d)", result);
    }

    if (result == SL_RESULT_SUCCESS)
    {
        result = (*g_engineObject)->Realize(g_engineObject, SL_BOOLEAN_FALSE);
        if (result != SL_RESULT_SUCCESS)
        {
            WriteTrace(TraceAudioInitShutdown, TraceError, "slCreateEngine->Realize failed (result: %d)", result);
        }
    }

    if (result == SL_RESULT_SUCCESS)
    {
        result = (*g_engineObject)->GetInterface(g_engineObject, SL_IID_ENGINE, &g_engineEngine);
        if (result != SL_RESULT_SUCCESS)
        {
            WriteTrace(TraceAudioInitShutdown, TraceError, "slCreateEngine->GetInterface failed (result: %d)", result);
        }
    }

    if (result == SL_RESULT_SUCCESS)
    {
        /* Output mix object */
        result = (*g_engineEngine)->CreateOutputMix(g_engineEngine, &g_outputMixObject, 0, NULL, NULL);
        if (result != SL_RESULT_SUCCESS)
        {
            WriteTrace(TraceAudioInitShutdown, TraceError, "slCreateEngine->CreateOutputMix failed (result: %d)", result);
        }
    }

    if (result == SL_RESULT_SUCCESS)
    {
        result = (*g_outputMixObject)->Realize(g_outputMixObject, SL_BOOLEAN_FALSE);
        if (result != SL_RESULT_SUCCESS)
        {
            WriteTrace(TraceAudioInitShutdown, TraceError, "g_outputMixObject->Realize failed (result: %d)", result);
        }
    }

    if (result == SL_RESULT_SUCCESS)
    {
        SLDataLocator_AndroidSimpleBufferQueue loc_bufq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, g_SecondaryBufferNbr };

        SLDataFormat_PCM format_pcm = { SL_DATAFORMAT_PCM,2, sample_rate, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
            (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT), SL_BYTEORDER_LITTLEENDIAN };

        SLDataSource audioSrc = { &loc_bufq, &format_pcm };

        /* Configure audio sink */
        SLDataLocator_OutputMix loc_outmix = { SL_DATALOCATOR_OUTPUTMIX, g_outputMixObject };
        SLDataSink audioSnk = { &loc_outmix, NULL };

        /* Create audio player */
        const SLInterfaceID ids1[] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
        const SLboolean req1[] = { SL_BOOLEAN_TRUE };
        result = (*g_engineEngine)->CreateAudioPlayer(g_engineEngine, &(g_playerObject), &audioSrc, &audioSnk, 1, ids1, req1);
        if (result != SL_RESULT_SUCCESS)
        {
            WriteTrace(TraceAudioInitShutdown, TraceError, "g_engineEngine->CreateAudioPlayer failed (result: %d)", result);
        }
    }

    /* Realize the player */
    if (result == SL_RESULT_SUCCESS)
    {
        result = (*g_playerObject)->Realize(g_playerObject, SL_BOOLEAN_FALSE);
        if (result != SL_RESULT_SUCCESS)
        {
            WriteTrace(TraceAudioInitShutdown, TraceError, "g_playerObject->Realize failed (result: %d)", result);
        }
    }

    /* Get the play interface */
    if (result == SL_RESULT_SUCCESS)
    {
        result = (*g_playerObject)->GetInterface(g_playerObject, SL_IID_PLAY, &(g_playerPlay));
        if (result != SL_RESULT_SUCCESS)
        {
            WriteTrace(TraceAudioInitShutdown, TraceError, "g_playerObject->GetInterface(SL_IID_PLAY) failed (result: %d)", result);
        }
    }

    /* Get the buffer queue interface */
    if (result == SL_RESULT_SUCCESS)
    {
        result = (*g_playerObject)->GetInterface(g_playerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &(g_bufferQueue));
        if (result != SL_RESULT_SUCCESS)
        {
            WriteTrace(TraceAudioInitShutdown, TraceError, "g_playerObject->GetInterface(SL_IID_ANDROIDSIMPLEBUFFERQUEUE) failed (result: %d)", result);
        }
    }

    /* register callback on the buffer queue */
    if (result == SL_RESULT_SUCCESS)
    {
        result = (*g_bufferQueue)->RegisterCallback(g_bufferQueue, queueCallback, &g_lock);
        if (result != SL_RESULT_SUCCESS)
        {
            WriteTrace(TraceAudioInitShutdown, TraceError, "bufferQueue->RegisterCallback() failed (result: %d)", result);
        }
    }

    /* set the player's state to playing */
    if (result == SL_RESULT_SUCCESS)
    {
        result = (*g_playerPlay)->SetPlayState(g_playerPlay, SL_PLAYSTATE_PLAYING);
        if (result != SL_RESULT_SUCCESS)
        {
            WriteTrace(TraceAudioInitShutdown, TraceError, "g_playerPlay->SetPlayState(SL_PLAYSTATE_PLAYING) failed (result: %d)", result);
        }
    }

    if (result != SL_RESULT_SUCCESS)
    {
        WriteTrace(TraceAudioInitShutdown, TraceNotice, "Couldn't open OpenSLES audio");
        CloseAudio();
        g_critical_failure = true;
    }
#endif
    WriteTrace(TraceAudioInitShutdown, TraceNotice, "Done");
    }

void OpenSLESDriver::AI_Shutdown(void)
{
    CloseAudio();
}

void OpenSLESDriver::AI_LenChanged(uint8_t *start, uint32_t length)
{
    WriteTrace(TraceAudioInterface, TraceDebug, "Start");
    WriteTrace(TraceAudioInterface, TraceDebug, "g_primaryBufferPos = 0x%X length = 0x%X g_primaryBufferBytes = %X", g_primaryBufferPos, length, g_primaryBufferBytes);
    if (g_primaryBufferPos + length < g_primaryBufferBytes)
    {
        unsigned int i;

        for (i = 0; i < length; i += 4)
        {
            if (g_SwapChannels == 0)
            {
                /* Left channel */
                g_primaryBuffer[g_primaryBufferPos + i] = start[i + 2];
                g_primaryBuffer[g_primaryBufferPos + i + 1] = start[i + 3];

                /* Right channel */
                g_primaryBuffer[g_primaryBufferPos + i + 2] = start[i];
                g_primaryBuffer[g_primaryBufferPos + i + 3] = start[i + 1];
            }
            else
            {
                /* Left channel */
                g_primaryBuffer[g_primaryBufferPos + i] = start[i];
                g_primaryBuffer[g_primaryBufferPos + i + 1] = start[i + 1];

                /* Right channel */
                g_primaryBuffer[g_primaryBufferPos + i + 2] = start[i + 2];
                g_primaryBuffer[g_primaryBufferPos + i + 3] = start[i + 3];
            }
        }
        g_primaryBufferPos += i;
    }
    else
    {
        WriteTrace(TraceAudioInterface, TraceDebug, "Audio primary buffer overflow. (g_primaryBufferPos: %d LenReg: %d g_primaryBufferBytes: %d)", g_primaryBufferPos, length, g_primaryBufferBytes);
    }

    uint32_t newsamplerate = g_OutputFreq * 100 / g_speed_factor;
    uint32_t oldsamplerate = g_GameFreq != 0 ? g_GameFreq : DEFAULT_FREQUENCY;

    while (g_primaryBufferPos >= ((g_secondaryBufferBytes * oldsamplerate) / newsamplerate))
    {
        WriteTrace(TraceAudioInterface, TraceDebug, "g_secondaryBufferBytes = %d", g_secondaryBufferBytes);
        WriteTrace(TraceAudioInterface, TraceDebug, "oldsamplerate = %d", oldsamplerate);
        WriteTrace(TraceAudioInterface, TraceDebug, "newsamplerate = %d", newsamplerate);
        WriteTrace(TraceAudioInterface, TraceDebug, "((g_secondaryBufferBytes * oldsamplerate) / newsamplerate) = %d", ((g_secondaryBufferBytes * oldsamplerate) / newsamplerate));
        WriteTrace(TraceAudioInterface, TraceDebug, "g_primaryBufferPos= %d", g_primaryBufferPos);
#ifdef ANDROID
        pthread_mutex_lock(&(g_lock.mutex));

        /* Wait for the next callback if no more output buffers available */
        while (g_lock.value == 0)
        {
            pthread_cond_wait(&(g_lock.cond), &(g_lock.mutex));
        }

        g_lock.value--;

        pthread_mutex_unlock(&(g_lock.mutex));
#endif
        WriteTrace(TraceAudioInterface, TraceDebug, "Finished with lock");

        // TODO: don't resample if speed_factor = 100 and newsamplerate ~= oldsamplerate
        int input_used = resample(g_primaryBuffer, g_primaryBufferPos, oldsamplerate, g_secondaryBuffers[g_secondaryBufferIndex], g_secondaryBufferBytes, newsamplerate);

#ifdef ANDROID
        (*g_bufferQueue)->Enqueue(g_bufferQueue, g_secondaryBuffers[g_secondaryBufferIndex], g_secondaryBufferBytes);
#endif
        memmove(g_primaryBuffer, &g_primaryBuffer[input_used], g_primaryBufferPos - input_used);
        g_primaryBufferPos -= input_used;

        g_secondaryBufferIndex++;

        if (g_secondaryBufferIndex > (g_SecondaryBufferNbr - 1))
        {
            g_secondaryBufferIndex = 0;
        }
    }
    WriteTrace(TraceAudioInterface, TraceDebug, "Done");
}