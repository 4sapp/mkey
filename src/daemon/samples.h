/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MKEY_SAMPLES_H
#define MKEY_SAMPLES_H

#include <glib.h>
#include <stdint.h>

#define MKEY_SAMPLE_RATE 48000

typedef enum {
    MKEY_KEY,
    MKEY_SPACE,
    MKEY_ENTER,
    MKEY_MOUSE_DOWN,
    MKEY_MOUSE_UP,
    MKEY_SAMPLE_COUNT
} MkeySampleId;

typedef struct {
    int16_t *frames;
    size_t count;
} MkeySample;

gboolean mkey_samples_load(MkeySample samples[MKEY_SAMPLE_COUNT], const char *directory,
                           GError **error);
void mkey_samples_clear(MkeySample samples[MKEY_SAMPLE_COUNT]);

#endif
