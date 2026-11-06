/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "settings.h"

#include <errno.h>
#include <glib/gstdio.h>
#include <inttypes.h>
#include <stdio.h>

void mkey_paths_init(MkeyPaths *paths, const char *data_dir)
{
    paths->config_dir = g_build_filename(g_get_user_config_dir(), "mkey", NULL);
    paths->settings_file = g_build_filename(paths->config_dir, "settings", NULL);
    paths->interface_file = g_build_filename(paths->config_dir, "interface.ini", NULL);
    paths->profiles_dir = g_build_filename(paths->config_dir, "profiles-v2", NULL);
    paths->data_dir =
        data_dir ? g_strdup(data_dir) : g_build_filename(g_get_user_data_dir(), "mkey", NULL);
    paths->sounds_dir = g_build_filename(paths->data_dir, "sounds", NULL);
}

void mkey_paths_clear(MkeyPaths *paths)
{
    g_free(paths->config_dir);
    g_free(paths->settings_file);
    g_free(paths->interface_file);
    g_free(paths->profiles_dir);
    g_free(paths->data_dir);
    g_free(paths->sounds_dir);
    *paths = (MkeyPaths){0};
}

gboolean mkey_paths_create(const MkeyPaths *paths, GError **error)
{
    if (g_mkdir_with_parents(paths->profiles_dir, 0700) == 0) {
        return TRUE;
    }

    g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                "Cannot create configuration directory: %s", g_strerror(errno));
    return FALSE;
}

MkeySettings mkey_settings_default(void)
{
    return (MkeySettings){
        .volume = 70, .keyboard_enabled = TRUE, .mouse_enabled = TRUE, .sound_pack = MKEY_PACK_2};
}

gboolean mkey_settings_load(const char *path, MkeySettings *settings, GError **error)
{
    g_autofree char *text = NULL;
    MkeySettings next = mkey_settings_default();
    int keyboard, mouse;
    int pack = MKEY_PACK_ORIGINAL;

    if (!g_file_get_contents(path, &text, NULL, error)) {
        return FALSE;
    }
    int fields = sscanf(text, "%d %d %d %d %" SCNu64 " %d %d", &next.volume, &keyboard, &mouse,
                        &next.theme, &next.preview_nonce, &next.preview_sample, &pack);
    if (fields < 6) {
        g_set_error_literal(error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                            "Invalid Mkey settings: expected at least six integers");
        return FALSE;
    }
    next.sound_pack = CLAMP(pack, MKEY_PACK_ORIGINAL, MKEY_PACK_COUNT - 1);

    next.volume = CLAMP(next.volume, 0, 100);
    next.keyboard_enabled = keyboard != 0;
    next.mouse_enabled = mouse != 0;
    next.theme = CLAMP(next.theme, 0, 10);
    next.preview_sample = CLAMP(next.preview_sample, 0, 4);
    *settings = next;
    return TRUE;
}

gboolean mkey_settings_save(const char *path, const MkeySettings *settings, GError **error)
{
    g_autofree char *text =
        g_strdup_printf("%d %d %d %d %" PRIu64 " %d %d\n", settings->volume,
                        settings->keyboard_enabled, settings->mouse_enabled, settings->theme,
                        settings->preview_nonce, settings->preview_sample, settings->sound_pack);
    return g_file_set_contents(path, text, -1, error);
}
