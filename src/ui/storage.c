/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui.h"

#include <stdio.h>
#include <string.h>

static void load_legacy_interface(MkeyUi *ui)
{
    g_autofree char *path = g_build_filename(ui->paths.config_dir, "interface", NULL);
    g_autofree char *contents = NULL;
    if (!g_file_get_contents(path, &contents, NULL, NULL)) {
        return;
    }
    g_auto(GStrv) lines = g_strsplit(contents, "\n", 4);
    int mono, zoom, hidden;
    if (g_strv_length(lines) >= 3 && sscanf(lines[0], "%d %d %d", &mono, &zoom, &hidden) == 3) {
        ui->appearance.monospace = mono != 0;
        ui->appearance.zoom = CLAMP(zoom, 60, 160);
        ui->appearance.hide_toolbar = hidden != 0;
        g_free(ui->appearance.title);
        ui->appearance.title = g_strdup(lines[2]);
    }
}

void mkey_ui_load(MkeyUi *ui)
{
    ui->settings = mkey_settings_default();
    ui->appearance =
        (MkeyAppearance){.title = g_strdup("Mkey 2"), .profile = g_strdup("default"), .zoom = 100};
    mkey_settings_load(ui->paths.settings_file, &ui->settings, NULL);

    g_autoptr(GKeyFile) file = g_key_file_new();
    if (!g_key_file_load_from_file(file, ui->paths.interface_file, G_KEY_FILE_NONE, NULL)) {
        load_legacy_interface(ui);
        return;
    }
    g_autofree char *title = g_key_file_get_string(file, "Interface", "title", NULL);
    g_autofree char *profile = g_key_file_get_string(file, "Interface", "profile", NULL);
    if (title) {
        g_free(ui->appearance.title);
        ui->appearance.title = g_utf8_make_valid(title, -1);
    }
    if (profile && !strchr(profile, '/') && !g_str_has_prefix(profile, ".")) {
        g_free(ui->appearance.profile);
        ui->appearance.profile = g_strdup(profile);
    }
    ui->appearance.zoom = CLAMP(g_key_file_get_integer(file, "Interface", "zoom", NULL), 60, 160);
    ui->appearance.monospace = g_key_file_get_boolean(file, "Interface", "monospace", NULL);
    ui->appearance.hide_toolbar = g_key_file_get_boolean(file, "Interface", "hide-toolbar", NULL);
}

void mkey_ui_save(MkeyUi *ui)
{
    if (ui->updating) {
        return;
    }
    g_autoptr(GError) error = NULL;
    if (!mkey_settings_save(ui->paths.settings_file, &ui->settings, &error)) {
        mkey_ui_error(ui, error->message);
        return;
    }
    g_autoptr(GKeyFile) file = g_key_file_new();
    g_key_file_set_string(file, "Interface", "title", ui->appearance.title);
    g_key_file_set_string(file, "Interface", "profile", ui->appearance.profile);
    g_key_file_set_integer(file, "Interface", "zoom", ui->appearance.zoom);
    g_key_file_set_boolean(file, "Interface", "monospace", ui->appearance.monospace);
    g_key_file_set_boolean(file, "Interface", "hide-toolbar", ui->appearance.hide_toolbar);
    if (!g_key_file_save_to_file(file, ui->paths.interface_file, &error)) {
        mkey_ui_error(ui, error->message);
        return;
    }
    mkey_ui_profile_save(ui);
}

void mkey_ui_preview(MkeyUi *ui, int sample)
{
    ui->settings.preview_nonce = (uint64_t)g_get_real_time();
    ui->settings.preview_sample = CLAMP(sample, 0, 4);
    mkey_ui_save(ui);
}
