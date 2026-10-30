/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MKEY_INPUT_H
#define MKEY_INPUT_H

#include "common/settings.h"
#include "samples.h"
#include <linux/input.h>
#include <poll.h>

#define MKEY_MAX_DEVICES 128

typedef struct {
    struct pollfd devices[MKEY_MAX_DEVICES];
    int64_t next_scan;
} MkeyInput;

typedef void (*MkeyInputCallback)(MkeySampleId sample, void *data);
void mkey_input_init(MkeyInput *input);
void mkey_input_clear(MkeyInput *input);
int mkey_input_sample(const struct input_event *event, const MkeySettings *settings);
gboolean mkey_input_poll(MkeyInput *input, const MkeySettings *settings, MkeyInputCallback callback,
                         void *data);

#endif
