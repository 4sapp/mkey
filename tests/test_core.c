/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "common/settings.h"
#include "daemon/input.h"
#include "daemon/mixer.h"
#include "daemon/samples.h"

#include <glib/gstdio.h>

static void settings_roundtrip(void)
{
    g_autofree char *directory = g_dir_make_tmp("mkey-settings-XXXXXX", NULL);
    g_autofree char *path = g_build_filename(directory, "settings", NULL);
    MkeySettings source = {.volume = 87,
                           .keyboard_enabled = FALSE,
                           .mouse_enabled = TRUE,
                           .theme = 7,
                           .preview_nonce = UINT64_MAX,
                           .preview_sample = 4,
                           .sound_pack = MKEY_PACK_2};
    MkeySettings loaded = {0};
    g_assert_true(mkey_settings_save(path, &source, NULL));
    g_assert_true(mkey_settings_load(path, &loaded, NULL));
    g_assert_cmpint(loaded.volume, ==, 87);
    g_assert_false(loaded.keyboard_enabled);
    g_assert_true(loaded.mouse_enabled);
    g_assert_cmpint(loaded.theme, ==, 7);
    g_assert_true(loaded.preview_nonce == UINT64_MAX);
    g_assert_cmpint(loaded.preview_sample, ==, 4);
    g_assert_cmpint(loaded.sound_pack, ==, MKEY_PACK_2);
    g_remove(path);
    g_rmdir(directory);
}

static void settings_invalid(void)
{
    g_autofree char *directory = g_dir_make_tmp("mkey-invalid-XXXXXX", NULL);
    g_autofree char *path = g_build_filename(directory, "settings", NULL);
    MkeySettings settings = mkey_settings_default();
    g_autoptr(GError) error = NULL;
    g_assert_true(g_file_set_contents(path, "80 1", -1, NULL));
    g_assert_false(mkey_settings_load(path, &settings, &error));
    g_assert_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL);
    g_assert_cmpint(settings.volume, ==, 70);
    g_assert_true(g_file_set_contents(path, "999 0 1 -8 1 88", -1, NULL));
    g_clear_error(&error);
    g_assert_true(mkey_settings_load(path, &settings, &error));
    g_assert_cmpint(settings.volume, ==, 100);
    g_assert_cmpint(settings.theme, ==, 0);
    g_assert_cmpint(settings.preview_sample, ==, 4);
    g_remove(path);
    g_rmdir(directory);
}

static void mixer_gain_and_completion(void)
{
    int16_t frames[] = {1000, -2000, 3000};
    MkeySample samples[MKEY_SAMPLE_COUNT] = {{.frames = frames, .count = 3}};
    MkeyMixer mixer;
    int16_t output[5];
    mkey_mixer_init(&mixer, samples);
    mkey_mixer_play(&mixer, MKEY_KEY);
    mkey_mixer_render(&mixer, 70, output, G_N_ELEMENTS(output));
    g_assert_cmpint(output[0], ==, 2000);
    g_assert_cmpint(output[1], ==, -4000);
    g_assert_cmpint(output[2], ==, 6000);
    g_assert_cmpint(output[3], ==, 0);
    g_assert_cmpint(output[4], ==, 0);
    mkey_mixer_play(&mixer, MKEY_KEY);
    mkey_mixer_render(&mixer, 0, output, G_N_ELEMENTS(output));
    for (size_t i = 0; i < G_N_ELEMENTS(output); i++) {
        g_assert_cmpint(output[i], ==, 0);
    }
    mkey_mixer_clear(&mixer);
}

static void mixer_saturates(void)
{
    int16_t frames[] = {30000, -30000};
    MkeySample samples[MKEY_SAMPLE_COUNT] = {{.frames = frames, .count = 2}};
    MkeyMixer mixer;
    int16_t output[2];
    mkey_mixer_init(&mixer, samples);
    mkey_mixer_play(&mixer, MKEY_KEY);
    mkey_mixer_play(&mixer, MKEY_KEY);
    mkey_mixer_render(&mixer, 100, output, 2);
    g_assert_cmpint(output[0], ==, INT16_MAX);
    g_assert_cmpint(output[1], ==, INT16_MIN);
    mkey_mixer_clear(&mixer);
}

static void mixer_invalid_and_empty(void)
{
    MkeySample samples[MKEY_SAMPLE_COUNT] = {0};
    MkeyMixer mixer;
    int16_t output[2] = {1, 1};
    mkey_mixer_init(&mixer, samples);
    mkey_mixer_play(&mixer, (MkeySampleId)-1);
    mkey_mixer_play(&mixer, MKEY_SAMPLE_COUNT);
    mkey_mixer_play(&mixer, MKEY_KEY);
    mkey_mixer_render(&mixer, 100, output, 2);
    g_assert_cmpint(output[0], ==, 0);
    g_assert_cmpint(output[1], ==, 0);
    mkey_mixer_clear(&mixer);
}

static void input_mapping(void)
{
    MkeySettings settings = mkey_settings_default();
    struct input_event event = {.type = EV_KEY, .code = KEY_A, .value = 1};
    g_assert_cmpint(mkey_input_sample(&event, &settings), ==, MKEY_KEY);
    event.code = KEY_SPACE;
    g_assert_cmpint(mkey_input_sample(&event, &settings), ==, MKEY_SPACE);
    event.code = KEY_KPENTER;
    g_assert_cmpint(mkey_input_sample(&event, &settings), ==, MKEY_ENTER);
    event.value = 2;
    g_assert_cmpint(mkey_input_sample(&event, &settings), ==, -1);
    event.value = 0;
    g_assert_cmpint(mkey_input_sample(&event, &settings), ==, -1);
    event.code = BTN_LEFT;
    g_assert_cmpint(mkey_input_sample(&event, &settings), ==, MKEY_MOUSE_UP);
    event.value = 1;
    g_assert_cmpint(mkey_input_sample(&event, &settings), ==, MKEY_MOUSE_DOWN);
    settings.mouse_enabled = FALSE;
    g_assert_cmpint(mkey_input_sample(&event, &settings), ==, -1);
    event.code = KEY_A;
    settings.keyboard_enabled = FALSE;
    g_assert_cmpint(mkey_input_sample(&event, &settings), ==, -1);
    event.type = EV_REL;
    g_assert_cmpint(mkey_input_sample(&event, &settings), ==, -1);
}

static void samples_reject_corrupt_file(void)
{
    g_autofree char *directory = g_dir_make_tmp("mkey-samples-XXXXXX", NULL);
    g_autofree char *path = g_build_filename(directory, "key.pcm", NULL);
    g_assert_true(g_file_set_contents(path, "x", 1, NULL));
    MkeySample samples[MKEY_SAMPLE_COUNT] = {0};
    g_autoptr(GError) error = NULL;
    g_assert_false(mkey_samples_load(samples, directory, &error));
    g_assert_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL);
    for (int i = 0; i < MKEY_SAMPLE_COUNT; i++) {
        g_assert_null(samples[i].frames);
    }
    g_remove(path);
    g_rmdir(directory);
}

static void settings_legacy_pack(void)
{
    g_autofree char *directory = g_dir_make_tmp("mkey-legacy-XXXXXX", NULL);
    g_autofree char *path = g_build_filename(directory, "settings", NULL);
    MkeySettings settings = mkey_settings_default();
    g_assert_cmpint(settings.sound_pack, ==, MKEY_PACK_2);
    g_assert_true(g_file_set_contents(path, "70 1 1 0 0 0\n", -1, NULL));
    g_assert_true(mkey_settings_load(path, &settings, NULL));
    g_assert_cmpint(settings.sound_pack, ==, MKEY_PACK_ORIGINAL);
    g_assert_true(g_file_set_contents(path, "70 1 1 0 0 0 99\n", -1, NULL));
    g_assert_true(mkey_settings_load(path, &settings, NULL));
    g_assert_cmpint(settings.sound_pack, ==, MKEY_PACK_2);
    g_remove(path);
    g_rmdir(directory);
}

static void mixer_pack_switch(void)
{
    int16_t original_frames[] = {1000, 2000, 3000};
    int16_t next_frames[] = {-1000, -2000};
    MkeySample original[MKEY_SAMPLE_COUNT] = {{.frames = original_frames, .count = 3}};
    MkeySample next[MKEY_SAMPLE_COUNT] = {{.frames = next_frames, .count = 2}};
    MkeyMixer mixer;
    int16_t output[3];
    mkey_mixer_init(&mixer, original);
    mkey_mixer_play(&mixer, MKEY_KEY);
    mkey_mixer_render(&mixer, 35, output, 1);
    g_assert_cmpint(output[0], ==, 1000);
    mkey_mixer_set_samples(&mixer, next);
    mkey_mixer_render(&mixer, 35, output, 3);
    g_assert_cmpint(output[0], ==, 0);
    mkey_mixer_play(&mixer, MKEY_KEY);
    mkey_mixer_render(&mixer, 35, output, 3);
    g_assert_cmpint(output[0], ==, -1000);
    g_assert_cmpint(output[1], ==, -2000);
    g_assert_cmpint(output[2], ==, 0);
    mkey_mixer_clear(&mixer);
}

static void sound_pack_paths(void)
{
    g_autofree char *original = mkey_sound_pack_directory("/sounds", MKEY_PACK_ORIGINAL);
    g_autofree char *next = mkey_sound_pack_directory("/sounds", MKEY_PACK_2);
    g_assert_cmpstr(original, ==, "/sounds");
    g_assert_cmpstr(next, ==, "/sounds/mkey2");
}

static void bundled_sound_packs(void)
{
    const char *data = g_getenv("MKEY_DATA_DIR");
    g_autofree char *base = g_build_filename(data ? data : "data", "sounds", NULL);
    for (int pack = 0; pack < MKEY_PACK_COUNT; pack++) {
        g_autofree char *directory = mkey_sound_pack_directory(base, (MkeySoundPack)pack);
        MkeySample samples[MKEY_SAMPLE_COUNT] = {0};
        g_assert_true(mkey_samples_load(samples, directory, NULL));
        for (int i = 0; i < MKEY_SAMPLE_COUNT; i++) {
            g_assert_cmpuint(samples[i].count, >, 0);
            gboolean nonzero = FALSE;
            for (size_t frame = 0; frame < samples[i].count; frame++) {
                nonzero |= samples[i].frames[frame] != 0;
            }
            g_assert_true(nonzero);
        }
        mkey_samples_clear(samples);
    }
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/samples/bundled-packs", bundled_sound_packs);
    g_test_add_func("/settings/legacy-pack", settings_legacy_pack);
    g_test_add_func("/mixer/pack-switch", mixer_pack_switch);
    g_test_add_func("/packs/paths", sound_pack_paths);
    g_test_add_func("/settings/roundtrip", settings_roundtrip);
    g_test_add_func("/settings/invalid", settings_invalid);
    g_test_add_func("/mixer/gain-and-completion", mixer_gain_and_completion);
    g_test_add_func("/mixer/saturation", mixer_saturates);
    g_test_add_func("/mixer/invalid-and-empty", mixer_invalid_and_empty);
    g_test_add_func("/input/mapping", input_mapping);
    g_test_add_func("/samples/corrupt-file", samples_reject_corrupt_file);
    return g_test_run();
}
