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

static gboolean select_pack(MkeyAudio *audio, MkeySample **samples, const char *base,
                            MkeySoundPack pack, GError **error)
{
    g_autofree char *directory = mkey_sound_pack_directory(base, pack);
    MkeySample *next = g_new0(MkeySample, MKEY_SAMPLE_COUNT);
    if (!mkey_samples_load(next, directory, error)) {
        g_free(next);
        return FALSE;
    }

    mkey_mixer_set_samples(&audio->mixer, next);
    mkey_samples_clear(*samples);
    g_free(*samples);
    *samples = next;
    g_printerr("Sound pack: %s\n", mkey_sound_pack_name(pack));
    return TRUE;
}

int main(int argc, char **argv)
{
    MkeyPaths paths = {0};
    MkeySettings settings = mkey_settings_default();
    MkeySample *samples = g_new0(MkeySample, MKEY_SAMPLE_COUNT);
    MkeyAudio audio;
    MkeyInput input;
    g_autoptr(GError) error = NULL;
    int result = EXIT_FAILURE;

    mkey_paths_init(&paths, g_getenv("MKEY_DATA_DIR"));
    const char *sounds_dir = argc > 1 ? argv[1] : paths.sounds_dir;
    mkey_settings_load(paths.settings_file, &settings, NULL);
    g_autofree char *pack_directory = mkey_sound_pack_directory(sounds_dir, settings.sound_pack);
    if (!mkey_samples_load(samples, pack_directory, &error) ||
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
        MkeySoundPack last_requested_pack = settings.sound_pack;
        mkey_input_init(&input);
        while (running) {
            if (mkey_settings_load(paths.settings_file, &settings, NULL)) {
                if (settings.sound_pack != last_requested_pack) {
                    last_requested_pack = settings.sound_pack;
                    g_autoptr(GError) pack_error = NULL;
                    if (!select_pack(&audio, &samples, sounds_dir, settings.sound_pack,
                                     &pack_error)) {
                        g_printerr("Cannot change sound pack: %s\n", pack_error->message);
                    }
                }
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
    g_free(samples);
    mkey_paths_clear(&paths);
    return result;
}
