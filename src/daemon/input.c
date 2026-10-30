/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "input.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define BITS_PER_LONG (8 * sizeof(unsigned long))
#define HAS_BIT(bits, code) (((bits)[(code) / BITS_PER_LONG] >> ((code) % BITS_PER_LONG)) & 1UL)

static int open_device(int index)
{
    char path[64];
    g_snprintf(path, sizeof(path), "/dev/input/event%d", index);
    int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }

    unsigned long keys[(KEY_MAX + BITS_PER_LONG) / BITS_PER_LONG] = {0};
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keys)), keys) < 0 ||
        !(HAS_BIT(keys, KEY_A) || HAS_BIT(keys, BTN_LEFT) || HAS_BIT(keys, BTN_RIGHT))) {
        close(fd);
        return -1;
    }
    char name[256] = {0};
    ioctl(fd, EVIOCGNAME(sizeof(name)), name);
    g_printerr("Listening: %s (%s)\n", path, name);
    return fd;
}

void mkey_input_init(MkeyInput *input)
{
    *input = (MkeyInput){0};
    for (size_t i = 0; i < MKEY_MAX_DEVICES; i++) {
        input->devices[i] = (struct pollfd){.fd = -1, .events = POLLIN};
    }
}

void mkey_input_clear(MkeyInput *input)
{
    for (size_t i = 0; i < MKEY_MAX_DEVICES; i++) {
        if (input->devices[i].fd >= 0) {
            close(input->devices[i].fd);
            input->devices[i].fd = -1;
        }
    }
}

int mkey_input_sample(const struct input_event *event, const MkeySettings *settings)
{
    if (event->type != EV_KEY) {
        return -1;
    }
    gboolean mouse =
        event->code == BTN_LEFT || event->code == BTN_RIGHT || event->code == BTN_MIDDLE;
    if (mouse && settings->mouse_enabled) {
        return event->value == 1 ? MKEY_MOUSE_DOWN : event->value == 0 ? MKEY_MOUSE_UP : -1;
    }
    if (event->code >= BTN_MISC || event->value != 1 || !settings->keyboard_enabled) {
        return -1;
    }
    switch (event->code) {
    case KEY_SPACE:
        return MKEY_SPACE;
    case KEY_ENTER:
    case KEY_KPENTER:
        return MKEY_ENTER;
    default:
        return MKEY_KEY;
    }
}

static gboolean read_events(int fd, const MkeySettings *settings, MkeyInputCallback callback,
                            void *data)
{
    struct input_event events[32];
    ssize_t size;

    while ((size = read(fd, events, sizeof(events))) > 0) {
        for (size_t i = 0; i < (size_t)size / sizeof(events[0]); i++) {
            int sample = mkey_input_sample(&events[i], settings);
            if (sample >= 0) {
                callback((MkeySampleId)sample, data);
            }
        }
    }
    return size != 0 && (errno == EAGAIN || errno == EINTR);
}

gboolean mkey_input_poll(MkeyInput *input, const MkeySettings *settings, MkeyInputCallback callback,
                         void *data)
{
    int64_t now = g_get_monotonic_time();
    if (now >= input->next_scan) {
        for (int i = 0; i < MKEY_MAX_DEVICES; i++) {
            if (input->devices[i].fd < 0) {
                input->devices[i].fd = open_device(i);
            }
        }
        input->next_scan = now + 2 * G_USEC_PER_SEC;
    }
    if (poll(input->devices, MKEY_MAX_DEVICES, 100) < 0) {
        return errno == EINTR;
    }
    for (size_t i = 0; i < MKEY_MAX_DEVICES; i++) {
        struct pollfd *device = &input->devices[i];
        if (device->fd < 0) {
            continue;
        }
        gboolean failed = (device->revents & (POLLERR | POLLHUP | POLLNVAL)) != 0;
        if (!failed && (device->revents & POLLIN)) {
            failed = !read_events(device->fd, settings, callback, data);
        }
        if (failed) {
            close(device->fd);
            device->fd = -1;
        }
    }
    return TRUE;
}
