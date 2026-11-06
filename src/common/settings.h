/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MKEY_SETTINGS_H
#define MKEY_SETTINGS_H

#include "sound_packs.h"
#include <glib.h>
#include <stdint.h>

typedef struct {
    int volume;
    gboolean keyboard_enabled;
    gboolean mouse_enabled;
    int theme;
    uint64_t preview_nonce;
    int preview_sample;
    MkeySoundPack sound_pack;
} MkeySettings;

typedef struct {
    char *config_dir;
    char *settings_file;
    char *interface_file;
    char *profiles_dir;
    char *data_dir;
    char *sounds_dir;
} MkeyPaths;

void mkey_paths_init(MkeyPaths *paths, const char *data_dir);
void mkey_paths_clear(MkeyPaths *paths);
gboolean mkey_paths_create(const MkeyPaths *paths, GError **error);
MkeySettings mkey_settings_default(void);
gboolean mkey_settings_load(const char *path, MkeySettings *settings, GError **error);
gboolean mkey_settings_save(const char *path, const MkeySettings *settings, GError **error);

#endif
