/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MKEY_MIXER_H
#define MKEY_MIXER_H

#include "samples.h"
#include <pthread.h>

#define MKEY_MAX_VOICES 64

typedef struct {
    MkeySampleId sample;
    size_t offset;
    gboolean active;
} MkeyVoice;

typedef struct {
    const MkeySample *samples;
    MkeyVoice voices[MKEY_MAX_VOICES];
    pthread_mutex_t mutex;
} MkeyMixer;

void mkey_mixer_init(MkeyMixer *mixer, const MkeySample *samples);
void mkey_mixer_clear(MkeyMixer *mixer);
void mkey_mixer_play(MkeyMixer *mixer, MkeySampleId sample);
void mkey_mixer_render(MkeyMixer *mixer, int volume, int16_t *output, size_t frames);

#endif
