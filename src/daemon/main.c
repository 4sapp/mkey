/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "audio.h"
#include "common/settings.h"
#include "input.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>

static volatile sig_atomic_t running = 1;

static void stop(int signal_number)
{
    (void)signal_number;
    running = 0;
}

static void play(MkeySampleId sample, void *data)
{
    MkeyAudio *audio = data;
    mkey_mixer_play(&audio->mixer, sample);
}

int main(int argc, char **argv)
{
    MkeyPaths paths = {0};
    MkeySettings settings = mkey_settings_default();
    MkeySample samples[MKEY_SAMPLE_COUNT] = {0};
    MkeyAudio audio;
    MkeyInput input;
    g_autoptr(GError) error = NULL;
    int result = EXIT_FAILURE;

    mkey_paths_init(&paths, g_getenv("MKEY_DATA_DIR"));
    const char *sounds_dir = argc > 1 ? argv[1] : paths.sounds_dir;
    mkey_settings_load(paths.settings_file, &settings, NULL);
    if (!mkey_samples_load(samples, sounds_dir, &error) ||
        !mkey_audio_start(&audio, samples, settings.volume, &error)) {
        g_printerr("%s\n", error->message);
        goto cleanup;
    }
    signal(SIGTERM, stop);
    signal(SIGINT, stop);

    if (argc > 2 && strcmp(argv[2], "--preview") == 0) {
        g_usleep(200000);
        for (int i = 0; i < MKEY_SAMPLE_COUNT; i++) {
            play((MkeySampleId)i, &audio);
            g_usleep(300000);
        }
        result = EXIT_SUCCESS;
    } else {
        uint64_t last_preview = settings.preview_nonce;
        mkey_input_init(&input);
        while (running) {
            if (mkey_settings_load(paths.settings_file, &settings, NULL)) {
                atomic_store(&audio.volume, settings.volume);
                if (settings.preview_nonce && settings.preview_nonce != last_preview) {
                    play((MkeySampleId)settings.preview_sample, &audio);
                }
                last_preview = settings.preview_nonce;
            }
            if (!mkey_input_poll(&input, &settings, play, &audio)) {
                g_printerr("Input polling failed\n");
                break;
            }
        }
        result = running ? EXIT_FAILURE : EXIT_SUCCESS;
        mkey_input_clear(&input);
    }
    mkey_audio_stop(&audio);
cleanup:
    mkey_samples_clear(samples);
    mkey_paths_clear(&paths);
    return result;
}
