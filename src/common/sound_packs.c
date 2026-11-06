/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "sound_packs.h"

const char *mkey_sound_pack_name(MkeySoundPack pack)
{
    switch (pack) {
    case MKEY_PACK_ORIGINAL:
        return "Mkey original";
    case MKEY_PACK_2:
        return "Mkey 2";
    default:
        return "Mkey original";
    }
}

char *mkey_sound_pack_directory(const char *base, MkeySoundPack pack)
{
    return pack == MKEY_PACK_2 ? g_build_filename(base, "mkey2", NULL) : g_strdup(base);
}
