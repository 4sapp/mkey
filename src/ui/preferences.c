/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui.h"

gboolean mkey_ui_autostart_enabled(void)
{
    g_autofree char *path =
        g_build_filename(g_get_user_config_dir(), "systemd", "user",
                         "graphical-session.target.wants", "mkey.service", NULL);
    return g_file_test(path, G_FILE_TEST_EXISTS);
}

gboolean mkey_ui_set_autostart(gboolean enabled, GError **error)
{
    const char *arguments[] = {"systemctl", "--user", enabled ? "enable" : "disable",
                               "mkey.service", NULL};
    int status;
    g_autofree char *stderr_text = NULL;
    if (!g_spawn_sync(NULL, (char **)arguments, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL,
                      &stderr_text, &status, error)) {
        return FALSE;
    }
    if (!g_spawn_check_wait_status(status, error)) {
        g_prefix_error(error, "%s", stderr_text ? stderr_text : "");
        return FALSE;
    }
    return TRUE;
}

static void autostart_changed(GObject *object, GParamSpec *property, gpointer data)
{
    (void)property;
    MkeyUi *ui = data;
    g_autoptr(GError) error = NULL;
    gboolean enabled = gtk_switch_get_active(GTK_SWITCH(object));
    if (!mkey_ui_set_autostart(enabled, &error)) {
        mkey_ui_error(ui, error->message);
    }
}

static void toolbar_changed(GObject *object, GParamSpec *property, gpointer data)
{
    (void)property;
    MkeyUi *ui = data;
    ui->appearance.hide_toolbar = gtk_switch_get_active(GTK_SWITCH(object));
    gtk_widget_set_visible(ui->toolbar, !ui->appearance.hide_toolbar);
    mkey_ui_save(ui);
}

static void close_clicked(GtkButton *button, gpointer data)
{
    (void)button;
    MkeyUi *ui = data;
    gtk_widget_set_visible(GTK_WIDGET(ui->preferences), FALSE);
}

static gboolean close_requested(GtkWindow *window, gpointer data)
{
    (void)data;
    gtk_widget_set_visible(GTK_WIDGET(window), FALSE);
    return TRUE;
}

static void restore_clicked(GtkButton *button, gpointer data)
{
    (void)button;
    mkey_ui_profile_restore(data);
}

static void preview_clicked(GtkButton *button, gpointer data)
{
    (void)button;
    mkey_ui_preview(data, 0);
}

static GtkWidget *preference_row(const char *name, const char *description, GtkWidget *control)
{
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    GtkWidget *text = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_hexpand(text, TRUE);
    gtk_box_append(GTK_BOX(text), mkey_label(name, "prefname"));
    if (description) {
        GtkWidget *detail = mkey_label(description, "detail");
        gtk_label_set_wrap(GTK_LABEL(detail), TRUE);
        gtk_box_append(GTK_BOX(text), detail);
    }
    gtk_widget_set_valign(control, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(row), text);
    gtk_box_append(GTK_BOX(row), control);
    return row;
}

void mkey_ui_show_preferences(MkeyUi *ui)
{
    if (ui->preferences) {
        gtk_window_present(ui->preferences);
        return;
    }
    ui->preferences = GTK_WINDOW(gtk_application_window_new(ui->application));
    gtk_window_set_title(ui->preferences, "Preferencias de Mkey 2");
    gtk_window_set_transient_for(ui->preferences, ui->window);
    gtk_window_set_modal(ui->preferences, TRUE);
    gtk_window_set_default_size(ui->preferences, 530, 330);
    gtk_widget_add_css_class(GTK_WIDGET(ui->preferences), "prefwindow");
    GtkWidget *hidden = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_visible(hidden, FALSE);
    gtk_window_set_titlebar(ui->preferences, hidden);

    GtkWidget *body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
    mkey_widget_margins(body, 20, 16);
    GtkWidget *heading = mkey_label("Preferencias de Mkey 2", "preftitle");
    gtk_widget_set_halign(heading, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(body), heading);
    GtkWidget *preview = gtk_button_new_with_label("Teclado y mouse");
    g_signal_connect(preview, "clicked", G_CALLBACK(preview_clicked), ui);
    gtk_box_append(GTK_BOX(body),
                   preference_row("Sonidos del video",
                                  "El paquete se elige en la ventana principal.", preview));
    GtkWidget *hide = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(hide), ui->appearance.hide_toolbar);
    g_signal_connect(hide, "notify::active", G_CALLBACK(toolbar_changed), ui);
    gtk_box_append(GTK_BOX(body), preference_row("Ocultar barra inferior (Ctrl+T)", NULL, hide));
    GtkWidget *start = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(start), mkey_ui_autostart_enabled());
    g_signal_connect(start, "notify::active", G_CALLBACK(autostart_changed), ui);
    gtk_box_append(GTK_BOX(body),
                   preference_row("Activar sonidos al iniciar sesión",
                                  "El sonido sigue activo al cerrar esta ventana.", start));
    GtkWidget *bottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *credit = mkey_label("Mkey 2 · Basado en Jorts", "detail");
    gtk_widget_set_hexpand(credit, TRUE);
    gtk_box_append(GTK_BOX(bottom), credit);
    GtkWidget *restore = gtk_button_new_with_label("Restaurar perfil");
    g_signal_connect(restore, "clicked", G_CALLBACK(restore_clicked), ui);
    gtk_box_append(GTK_BOX(bottom), restore);
    GtkWidget *close = gtk_button_new_with_label("Cerrar");
    g_signal_connect(close, "clicked", G_CALLBACK(close_clicked), ui);
    gtk_box_append(GTK_BOX(bottom), close);
    gtk_box_append(GTK_BOX(body), bottom);
    gtk_window_set_child(ui->preferences, body);
    g_signal_connect(ui->preferences, "close-request", G_CALLBACK(close_requested), ui);
    gtk_window_present(ui->preferences);
}
