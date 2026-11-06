/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "mixer.h"

#include <string.h>

/* The reference samples were prepared at 35% gain. Keep their relative levels. */
#define SAMPLE_REFERENCE_VOLUME 35.0

void mkey_mixer_init(MkeyMixer *mixer, const MkeySample *samples)
{
    *mixer = (MkeyMixer){.samples = samples};
    pthread_mutex_init(&mixer->mutex, NULL);
}

void mkey_mixer_set_samples(MkeyMixer *mixer, const MkeySample *samples)
{
    pthread_mutex_lock(&mixer->mutex);
    memset(mixer->voices, 0, sizeof(mixer->voices));
    mixer->samples = samples;
    pthread_mutex_unlock(&mixer->mutex);
}

void mkey_mixer_clear(MkeyMixer *mixer)
{
    pthread_mutex_destroy(&mixer->mutex);
}

void mkey_mixer_play(MkeyMixer *mixer, MkeySampleId sample)
{
    if (sample < 0 || sample >= MKEY_SAMPLE_COUNT) {
        return;
    }
    pthread_mutex_lock(&mixer->mutex);
    for (size_t i = 0; i < MKEY_MAX_VOICES; i++) {
        if (!mixer->voices[i].active) {
            mixer->voices[i] = (MkeyVoice){.sample = sample, .active = TRUE};
            break;
        }
    }
    pthread_mutex_unlock(&mixer->mutex);
}

void mkey_mixer_render(MkeyMixer *mixer, int volume, int16_t *output, size_t frames)
{
    double gain = CLAMP(volume, 0, 100) / SAMPLE_REFERENCE_VOLUME;

    pthread_mutex_lock(&mixer->mutex);
    for (size_t frame = 0; frame < frames; frame++) {
        int mixed = 0;
        for (size_t i = 0; i < MKEY_MAX_VOICES; i++) {
            MkeyVoice *voice = &mixer->voices[i];
            if (!voice->active) {
                continue;
            }
            const MkeySample *sample = &mixer->samples[voice->sample];
            if (voice->offset < sample->count) {
                mixed += sample->frames[voice->offset++];
            }
            voice->active = voice->offset < sample->count;
        }
        int scaled = (int)(mixed * gain);
        output[frame] = (int16_t)CLAMP(scaled, INT16_MIN, INT16_MAX);
    }
    pthread_mutex_unlock(&mixer->mutex);
}
