/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui.h"

typedef struct {
    const char *name;
    const char *background;
    const char *foreground;
} NoteTheme;

static const NoteTheme themes[] = {
    {"Arándano", "#8fd5fa", "#093878"}, {"Menta", "#b4eadb", "#155745"},
    {"Lima", "#d5ed9e", "#35570d"},     {"Banana", "#ffec9c", "#725311"},
    {"Naranja", "#ffd0a1", "#793808"},  {"Fresa", "#ffb6b0", "#7a2527"},
    {"Chicle", "#f6c1e2", "#742c58"},   {"Uva", "#d7c3f0", "#492b77"},
    {"Cacao", "#d7bca7", "#53301d"},    {"Pizarra", "#c4ccd3", "#2d3d4b"},
    {"Latte", "#f1e6d1", "#5a4935"}};

void mkey_ui_apply_style(MkeyUi *ui)
{
    const NoteTheme *theme = &themes[ui->settings.theme];
    g_autoptr(GString) css = g_string_new(NULL);
    g_string_append_printf(css, "@define-color note_bg %s; @define-color note_ink %s;\n",
                           theme->background, theme->foreground);
    g_autofree char *path = g_build_filename(ui->paths.data_dir, "style.css", NULL);
    g_autofree char *base = NULL;
    if (!g_file_get_contents(path, &base, NULL, NULL)) {
        g_warning("Cannot read %s", path);
        return;
    }
    g_string_append(css, base);
    g_string_append_printf(
        css,
        ".mkey editablelabel {font-size:%dpx;} .mkey .body label {font-size:%dpx;} "
        ".mkey .body label.detail {font-size:12px;} .mkey .body {font-family:%s;} "
        ".mkey button.test label {font-size:14px;}\n",
        18 * ui->appearance.zoom / 100, 20 * ui->appearance.zoom / 100,
        ui->appearance.monospace ? "monospace" : "sans-serif");
    for (size_t i = 0; i < G_N_ELEMENTS(themes); i++) {
        g_string_append_printf(css, "#color%zu {background:%s;}\n", i, themes[i].background);
    }
    gtk_css_provider_load_from_string(ui->css, css->str);
}

static void theme_clicked(GtkButton *button, gpointer data)
{
    MkeyUi *ui = data;
    ui->settings.theme = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "theme-index"));
    mkey_ui_apply_style(ui);
    mkey_ui_save(ui);
}

static void monospace_changed(GObject *object, GParamSpec *property, gpointer data)
{
    (void)property;
    MkeyUi *ui = data;
    if (!ui->updating) {
        ui->appearance.monospace = gtk_switch_get_active(GTK_SWITCH(object));
        mkey_ui_apply_style(ui);
        mkey_ui_save(ui);
    }
}

static void zoom_clicked(GtkButton *button, gpointer data)
{
    MkeyUi *ui = data;
    int delta = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "zoom-delta"));
    ui->appearance.zoom = CLAMP(ui->appearance.zoom + delta, 60, 160);
    mkey_ui_refresh(ui);
    mkey_ui_save(ui);
}

static void preferences_clicked(GtkButton *button, gpointer data)
{
    (void)button;
    MkeyUi *ui = data;
    gtk_popover_popdown(ui->appearance_menu);
    mkey_ui_show_preferences(ui);
}

GtkPopover *mkey_ui_appearance_menu(MkeyUi *ui)
{
    GtkWidget *popover = gtk_popover_new();
    gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_TOP);
    GtkWidget *body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    GtkWidget *palette = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    for (size_t i = 0; i < G_N_ELEMENTS(themes); i++) {
        GtkWidget *button = gtk_button_new();
        g_autofree char *id = g_strdup_printf("color%zu", i);
        gtk_widget_set_name(button, id);
        gtk_widget_set_tooltip_text(button, themes[i].name);
        gtk_widget_add_css_class(button, "swatch");
        g_object_set_data(G_OBJECT(button), "theme-index", GINT_TO_POINTER(i));
        g_signal_connect(button, "clicked", G_CALLBACK(theme_clicked), ui);
        gtk_box_append(GTK_BOX(palette), button);
    }
    gtk_box_append(GTK_BOX(body), palette);
    gtk_box_append(GTK_BOX(body), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    ui->monospace_switch = GTK_SWITCH(gtk_switch_new());
    g_signal_connect(ui->monospace_switch, "notify::active", G_CALLBACK(monospace_changed), ui);
    gtk_box_append(GTK_BOX(body),
                   mkey_row("Tipografía monoespaciada", GTK_WIDGET(ui->monospace_switch)));

    GtkWidget *zoom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    const int deltas[] = {-20, 20};
    ui->zoom_label = GTK_LABEL(gtk_label_new(NULL));
    for (size_t i = 0; i < G_N_ELEMENTS(deltas); i++) {
        GtkWidget *button = gtk_button_new_with_label(i == 0 ? "−" : "+");
        g_object_set_data(G_OBJECT(button), "zoom-delta", GINT_TO_POINTER(deltas[i]));
        g_signal_connect(button, "clicked", G_CALLBACK(zoom_clicked), ui);
        gtk_box_append(GTK_BOX(zoom), button);
        if (i == 0) {
            gtk_box_append(GTK_BOX(zoom), GTK_WIDGET(ui->zoom_label));
        }
    }
    gtk_box_append(GTK_BOX(body), mkey_row("Tamaño del texto", zoom));
    gtk_box_append(GTK_BOX(body), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    GtkWidget *preferences = gtk_button_new_with_label("Preferencias de Mkey…");
    g_signal_connect(preferences, "clicked", G_CALLBACK(preferences_clicked), ui);
    gtk_box_append(GTK_BOX(body), preferences);
    gtk_popover_set_child(GTK_POPOVER(popover), body);
    return GTK_POPOVER(popover);
}
