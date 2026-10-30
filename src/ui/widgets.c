/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui.h"

GtkWidget *mkey_label(const char *text, const char *css_class)
{
    GtkWidget *label = gtk_label_new(text);
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    if (css_class) {
        gtk_widget_add_css_class(label, css_class);
    }
    return label;
}

GtkWidget *mkey_row(const char *text, GtkWidget *control)
{
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    GtkWidget *label = mkey_label(text, NULL);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_widget_set_valign(control, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(row), label);
    gtk_box_append(GTK_BOX(row), control);
    return row;
}

void mkey_widget_margins(GtkWidget *widget, int horizontal, int vertical)
{
    gtk_widget_set_margin_start(widget, horizontal);
    gtk_widget_set_margin_end(widget, horizontal);
    gtk_widget_set_margin_top(widget, vertical);
    gtk_widget_set_margin_bottom(widget, vertical);
}

void mkey_ui_error(MkeyUi *ui, const char *message)
{
    g_printerr("Mkey: %s\n", message);
    if (ui->window) {
        GtkAlertDialog *dialog = gtk_alert_dialog_new("%s", message);
        gtk_alert_dialog_show(dialog, ui->window);
        g_object_unref(dialog);
    }
}
