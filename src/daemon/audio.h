/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MKEY_AUDIO_H
#define MKEY_AUDIO_H

#include "mixer.h"
#include <stdatomic.h>

typedef struct {
    MkeyMixer mixer;
    pthread_t thread;
    atomic_bool running;
    atomic_int volume;
} MkeyAudio;

gboolean mkey_audio_start(MkeyAudio *audio, const MkeySample *samples, int volume, GError **error);
void mkey_audio_stop(MkeyAudio *audio);

#endif
