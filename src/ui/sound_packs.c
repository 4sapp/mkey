/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui.h"

static void pack_changed(GObject *object, GParamSpec *property, gpointer data)
{
    (void)property;
    MkeyUi *ui = data;
    if (ui->updating) {
        return;
    }
    guint selected = gtk_drop_down_get_selected(GTK_DROP_DOWN(object));
    if (selected < MKEY_PACK_COUNT) {
        ui->settings.sound_pack = (MkeySoundPack)selected;
        mkey_ui_preview(ui, 0);
    }
}

GtkWidget *mkey_ui_sound_pack_selector(MkeyUi *ui)
{
    const char *names[MKEY_PACK_COUNT + 1] = {0};
    for (int i = 0; i < MKEY_PACK_COUNT; i++) {
        names[i] = mkey_sound_pack_name((MkeySoundPack)i);
    }
    ui->sound_pack_selector = GTK_DROP_DOWN(gtk_drop_down_new_from_strings(names));
    gtk_drop_down_set_selected(ui->sound_pack_selector, ui->settings.sound_pack);
    gtk_widget_set_tooltip_text(GTK_WIDGET(ui->sound_pack_selector),
                                "Elegir sonidos de teclado y mouse");
    g_signal_connect(ui->sound_pack_selector, "notify::selected", G_CALLBACK(pack_changed), ui);
    return GTK_WIDGET(ui->sound_pack_selector);
}
