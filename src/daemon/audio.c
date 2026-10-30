/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "audio.h"

#include <pulse/error.h>
#include <pulse/simple.h>

#define AUDIO_BLOCK_FRAMES 240

static void *audio_worker(void *data)
{
    MkeyAudio *audio = data;
    const pa_sample_spec format = {
        .format = PA_SAMPLE_S16NE, .rate = MKEY_SAMPLE_RATE, .channels = 1};
    const pa_buffer_attr buffering = {.maxlength = UINT32_MAX,
                                      .tlength = 960,
                                      .prebuf = 0,
                                      .minreq = 480,
                                      .fragsize = UINT32_MAX};
    int16_t output[AUDIO_BLOCK_FRAMES];

    while (atomic_load(&audio->running)) {
        int error;
        pa_simple *stream = pa_simple_new(NULL, "mkey", PA_STREAM_PLAYBACK, NULL,
                                          "Keyboard and mouse", &format, NULL, &buffering, &error);
        if (!stream) {
            g_printerr("Audio connection failed: %s; retrying\n", pa_strerror(error));
            g_usleep(G_USEC_PER_SEC);
            continue;
        }
        while (atomic_load(&audio->running)) {
            mkey_mixer_render(&audio->mixer, atomic_load(&audio->volume), output,
                              G_N_ELEMENTS(output));
            if (pa_simple_write(stream, output, sizeof(output), &error) < 0) {
                g_printerr("Audio write failed: %s\n", pa_strerror(error));
                break;
            }
        }
        pa_simple_free(stream);
    }
    return NULL;
}

gboolean mkey_audio_start(MkeyAudio *audio, const MkeySample *samples, int volume, GError **error)
{
    mkey_mixer_init(&audio->mixer, samples);
    atomic_init(&audio->running, TRUE);
    atomic_init(&audio->volume, volume);
    int status = pthread_create(&audio->thread, NULL, audio_worker, audio);
    if (status != 0) {
        g_set_error(error, G_THREAD_ERROR, G_THREAD_ERROR_AGAIN, "Cannot start audio: %s",
                    g_strerror(status));
        mkey_mixer_clear(&audio->mixer);
        return FALSE;
    }
    return TRUE;
}

void mkey_audio_stop(MkeyAudio *audio)
{
    atomic_store(&audio->running, FALSE);
    pthread_join(audio->thread, NULL);
    mkey_mixer_clear(&audio->mixer);
}
