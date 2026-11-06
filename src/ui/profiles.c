/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui.h"

#include <errno.h>
#include <glib/gstdio.h>
#include <string.h>

static char *profile_path(MkeyUi *ui, const char *id)
{
    if (!id || !*id || strchr(id, '/') || id[0] == '.') {
        return NULL;
    }
    return g_build_filename(ui->paths.profiles_dir, id, NULL);
}

void mkey_ui_profile_save(MkeyUi *ui)
{
    g_autofree char *path = profile_path(ui, ui->appearance.profile);
    g_autoptr(GKeyFile) profile = g_key_file_new();
    g_autoptr(GError) error = NULL;
    if (!path) {
        return;
    }
    g_key_file_set_string(profile, "Profile", "title", ui->appearance.title);
    g_key_file_set_integer(profile, "Profile", "volume", ui->settings.volume);
    g_key_file_set_integer(profile, "Profile", "sound-pack", ui->settings.sound_pack);
    g_key_file_set_boolean(profile, "Profile", "keyboard", ui->settings.keyboard_enabled);
    g_key_file_set_boolean(profile, "Profile", "mouse", ui->settings.mouse_enabled);
    g_key_file_set_integer(profile, "Profile", "theme", ui->settings.theme);
    g_key_file_set_integer(profile, "Profile", "zoom", ui->appearance.zoom);
    g_key_file_set_boolean(profile, "Profile", "monospace", ui->appearance.monospace);
    if (!g_key_file_save_to_file(profile, path, &error)) {
        mkey_ui_error(ui, error->message);
    }
}

gboolean mkey_ui_profile_load(MkeyUi *ui, const char *id)
{
    g_autofree char *selected_id = g_strdup(id);
    g_autofree char *path = profile_path(ui, selected_id);
    g_autoptr(GKeyFile) profile = g_key_file_new();
    if (!path || !g_key_file_load_from_file(profile, path, G_KEY_FILE_NONE, NULL)) {
        return FALSE;
    }
    g_autofree char *title = g_key_file_get_string(profile, "Profile", "title", NULL);
    if (!title) {
        return FALSE;
    }
    ui->settings.sound_pack = CLAMP(g_key_file_get_integer(profile, "Profile", "sound-pack", NULL),
                                    MKEY_PACK_ORIGINAL, MKEY_PACK_COUNT - 1);
    ui->settings.volume = CLAMP(g_key_file_get_integer(profile, "Profile", "volume", NULL), 0, 100);
    ui->settings.keyboard_enabled = g_key_file_get_boolean(profile, "Profile", "keyboard", NULL);
    ui->settings.mouse_enabled = g_key_file_get_boolean(profile, "Profile", "mouse", NULL);
    ui->settings.theme = CLAMP(g_key_file_get_integer(profile, "Profile", "theme", NULL), 0, 10);
    ui->appearance.zoom = CLAMP(g_key_file_get_integer(profile, "Profile", "zoom", NULL), 60, 160);
    ui->appearance.monospace = g_key_file_get_boolean(profile, "Profile", "monospace", NULL);
    g_free(ui->appearance.title);
    ui->appearance.title = g_utf8_make_valid(title, -1);
    g_free(ui->appearance.profile);
    ui->appearance.profile = g_strdup(selected_id);
    mkey_ui_refresh(ui);
    mkey_ui_save(ui);
    return TRUE;
}

static void select_profile(GtkButton *button, gpointer data)
{
    MkeyUi *ui = data;
    const char *id = g_object_get_data(G_OBJECT(button), "profile-id");
    mkey_ui_profile_load(ui, id);
    gtk_popover_popdown(ui->profiles_menu);
}

void mkey_ui_profiles_rebuild(MkeyUi *ui)
{
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_append(GTK_BOX(content), mkey_label("Perfiles de sonido", NULL));
    g_autoptr(GDir) directory = g_dir_open(ui->paths.profiles_dir, 0, NULL);
    const char *name;
    while (directory && (name = g_dir_read_name(directory))) {
        g_autofree char *path = profile_path(ui, name);
        g_autoptr(GKeyFile) profile = g_key_file_new();
        if (!path || !g_key_file_load_from_file(profile, path, G_KEY_FILE_NONE, NULL)) {
            continue;
        }
        g_autofree char *title = g_key_file_get_string(profile, "Profile", "title", NULL);
        if (!title) {
            continue;
        }
        GtkWidget *button = gtk_button_new_with_label(title);
        g_object_set_data_full(G_OBJECT(button), "profile-id", g_strdup(name), g_free);
        g_signal_connect(button, "clicked", G_CALLBACK(select_profile), ui);
        gtk_box_append(GTK_BOX(content), button);
    }
    gtk_popover_set_child(ui->profiles_menu, content);
}

void mkey_ui_profile_new(MkeyUi *ui)
{
    mkey_ui_profile_save(ui);
    g_free(ui->appearance.profile);
    ui->appearance.profile = g_uuid_string_random();
    g_free(ui->appearance.title);
    ui->appearance.title = g_strdup("Mis nuevos sonidos 🎵");
    mkey_ui_refresh(ui);
    mkey_ui_save(ui);
    mkey_ui_profiles_rebuild(ui);
}

void mkey_ui_profile_delete(MkeyUi *ui)
{
    g_autofree char *path = profile_path(ui, ui->appearance.profile);
    g_autofree char *backup = g_build_filename(ui->paths.config_dir, "deleted-profile.ini", NULL);
    if (!path || g_rename(path, backup) != 0) {
        mkey_ui_error(ui, g_strerror(errno));
        return;
    }
    ui->settings = mkey_settings_default();
    g_free(ui->appearance.profile);
    ui->appearance.profile = g_uuid_string_random();
    g_free(ui->appearance.title);
    ui->appearance.title = g_strdup("Mis sonidos");
    ui->appearance.zoom = 100;
    ui->appearance.monospace = FALSE;
    mkey_ui_refresh(ui);
    mkey_ui_save(ui);
    mkey_ui_profiles_rebuild(ui);
}

void mkey_ui_profile_restore(MkeyUi *ui)
{
    g_autofree char *backup = g_build_filename(ui->paths.config_dir, "deleted-profile.ini", NULL);
    g_autofree char *id = g_uuid_string_random();
    g_autofree char *target = profile_path(ui, id);
    if (g_rename(backup, target) == 0) {
        mkey_ui_profile_load(ui, id);
        mkey_ui_profiles_rebuild(ui);
    }
}
