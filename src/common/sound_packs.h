/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MKEY_SOUND_PACKS_H
#define MKEY_SOUND_PACKS_H

#include <glib.h>

typedef enum { MKEY_PACK_ORIGINAL, MKEY_PACK_2, MKEY_PACK_COUNT } MkeySoundPack;

const char *mkey_sound_pack_name(MkeySoundPack pack);
char *mkey_sound_pack_directory(const char *base, MkeySoundPack pack);

#endif
