/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "samples.h"

static const char *const filenames[MKEY_SAMPLE_COUNT] = {"key.pcm", "space.pcm", "enter.pcm",
                                                         "mouse-down.pcm", "mouse-up.pcm"};

gboolean mkey_samples_load(MkeySample samples[MKEY_SAMPLE_COUNT], const char *directory,
                           GError **error)
{
    for (int i = 0; i < MKEY_SAMPLE_COUNT; i++) {
        g_autofree char *path = g_build_filename(directory, filenames[i], NULL);
        g_autofree char *data = NULL;
        size_t size;

        if (!g_file_get_contents(path, &data, &size, error)) {
            mkey_samples_clear(samples);
            return FALSE;
        }
        if (size < 2 || size > MKEY_SAMPLE_RATE * sizeof(int16_t) || size % 2 != 0) {
            g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "Invalid PCM sample: %s", path);
            mkey_samples_clear(samples);
            return FALSE;
        }

        samples[i].count = size / sizeof(int16_t);
        samples[i].frames = (int16_t *)g_steal_pointer(&data);
        for (size_t frame = 0; frame < samples[i].count; frame++) {
            samples[i].frames[frame] = GINT16_FROM_LE(samples[i].frames[frame]);
        }
    }
    return TRUE;
}

void mkey_samples_clear(MkeySample samples[MKEY_SAMPLE_COUNT])
{
    for (int i = 0; i < MKEY_SAMPLE_COUNT; i++) {
        g_clear_pointer(&samples[i].frames, g_free);
        samples[i].count = 0;
    }
}
