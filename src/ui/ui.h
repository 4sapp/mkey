/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MKEY_UI_H
#define MKEY_UI_H

#include "common/settings.h"
#include <gtk/gtk.h>

typedef struct {
    char *title;
    char *profile;
    gboolean monospace;
    gboolean hide_toolbar;
    int zoom;
} MkeyAppearance;

typedef struct {
    GtkApplication *application;
    GtkWindow *window;
    GtkWindow *preferences;
    GtkCssProvider *css;
    GtkLabel *volume_label;
    GtkRange *volume_scale;
    GtkSwitch *keyboard_switch;
    GtkSwitch *mouse_switch;
    GtkSwitch *monospace_switch;
    GtkLabel *zoom_label;
    GtkEditable *title;
    GtkWidget *toolbar;
    GtkPopover *appearance_menu;
    GtkPopover *profiles_menu;
    MkeyPaths paths;
    MkeySettings settings;
    MkeyAppearance appearance;
    gboolean updating;
    const char *screenshot;
    gboolean smoke_test;
} MkeyUi;

void mkey_ui_load(MkeyUi *ui);
void mkey_ui_save(MkeyUi *ui);
void mkey_ui_preview(MkeyUi *ui, int sample);
void mkey_ui_error(MkeyUi *ui, const char *message);
void mkey_ui_window_create(MkeyUi *ui);
void mkey_ui_refresh(MkeyUi *ui);
void mkey_ui_apply_style(MkeyUi *ui);
GtkPopover *mkey_ui_appearance_menu(MkeyUi *ui);
void mkey_ui_show_preferences(MkeyUi *ui);
void mkey_ui_profiles_rebuild(MkeyUi *ui);
void mkey_ui_profile_save(MkeyUi *ui);
void mkey_ui_profile_new(MkeyUi *ui);
void mkey_ui_profile_delete(MkeyUi *ui);
void mkey_ui_profile_restore(MkeyUi *ui);
gboolean mkey_ui_profile_load(MkeyUi *ui, const char *id);
gboolean mkey_ui_autostart_enabled(void);
gboolean mkey_ui_set_autostart(gboolean enabled, GError **error);
GtkWidget *mkey_label(const char *text, const char *css_class);
GtkWidget *mkey_row(const char *text, GtkWidget *control);
void mkey_widget_margins(GtkWidget *widget, int horizontal, int vertical);

#endif
