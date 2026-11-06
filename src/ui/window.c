/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui.h"

void mkey_ui_refresh(MkeyUi *ui)
{
    ui->updating = TRUE;
    gtk_drop_down_set_selected(ui->sound_pack_selector, ui->settings.sound_pack);
    gtk_range_set_value(ui->volume_scale, ui->settings.volume);
    gtk_switch_set_active(ui->keyboard_switch, ui->settings.keyboard_enabled);
    gtk_switch_set_active(ui->mouse_switch, ui->settings.mouse_enabled);
    gtk_switch_set_active(ui->monospace_switch, ui->appearance.monospace);
    gtk_editable_set_text(ui->title, ui->appearance.title);
    gtk_window_set_title(ui->window, ui->appearance.title);
    g_autofree char *volume = g_strdup_printf("%d %%", ui->settings.volume);
    g_autofree char *zoom = g_strdup_printf("%d %%", ui->appearance.zoom);
    gtk_label_set_text(ui->volume_label, volume);
    gtk_label_set_text(ui->zoom_label, zoom);
    gtk_widget_set_visible(ui->toolbar, !ui->appearance.hide_toolbar);
    mkey_ui_apply_style(ui);
    ui->updating = FALSE;
}

static void volume_changed(GtkRange *range, gpointer data)
{
    MkeyUi *ui = data;
    if (ui->updating) {
        return;
    }
    ui->settings.volume = (int)gtk_range_get_value(range);
    g_autofree char *text = g_strdup_printf("%d %%", ui->settings.volume);
    gtk_label_set_text(ui->volume_label, text);
    mkey_ui_save(ui);
}

static void device_changed(GObject *object, GParamSpec *property, gpointer data)
{
    (void)object;
    (void)property;
    MkeyUi *ui = data;
    if (!ui->updating) {
        ui->settings.keyboard_enabled = gtk_switch_get_active(ui->keyboard_switch);
        ui->settings.mouse_enabled = gtk_switch_get_active(ui->mouse_switch);
        mkey_ui_save(ui);
    }
}

static void title_changed(GObject *object, GParamSpec *property, gpointer data)
{
    (void)object;
    (void)property;
    MkeyUi *ui = data;
    if (!ui->updating) {
        g_free(ui->appearance.title);
        ui->appearance.title = g_strdup(gtk_editable_get_text(ui->title));
        gtk_window_set_title(ui->window, ui->appearance.title);
        mkey_ui_save(ui);
        mkey_ui_profiles_rebuild(ui);
    }
}

static void preview_clicked(GtkButton *button, gpointer data)
{
    int sample = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "sample-id"));
    mkey_ui_preview(data, sample);
}

static void new_clicked(GtkButton *button, gpointer data)
{
    (void)button;
    mkey_ui_profile_new(data);
}

static void delete_clicked(GtkButton *button, gpointer data)
{
    (void)button;
    mkey_ui_profile_delete(data);
}

static void emoji_picked(GtkEmojiChooser *chooser, const char *emoji, gpointer data)
{
    (void)chooser;
    MkeyUi *ui = data;
    g_autofree char *title = g_strdup_printf("%s %s", ui->appearance.title, emoji);
    gtk_editable_set_text(ui->title, title);
}

static gboolean close_requested(GtkWindow *window, gpointer data)
{
    (void)window;
    MkeyUi *ui = data;
    g_application_quit(G_APPLICATION(ui->application));
    return TRUE;
}

static gboolean key_pressed(GtkEventControllerKey *controller, guint key, guint code,
                            GdkModifierType modifiers, gpointer data)
{
    (void)controller;
    (void)code;
    MkeyUi *ui = data;
    if (!(modifiers & GDK_CONTROL_MASK)) {
        return FALSE;
    }
    switch (key) {
    case GDK_KEY_t:
        ui->appearance.hide_toolbar = !ui->appearance.hide_toolbar;
        gtk_widget_set_visible(ui->toolbar, !ui->appearance.hide_toolbar);
        mkey_ui_save(ui);
        break;
    case GDK_KEY_n:
        mkey_ui_profile_new(ui);
        break;
    case GDK_KEY_w:
        g_application_quit(G_APPLICATION(ui->application));
        break;
    case GDK_KEY_o:
    case GDK_KEY_g:
        gtk_popover_popup(ui->appearance_menu);
        break;
    case GDK_KEY_l:
        gtk_widget_grab_focus(GTK_WIDGET(ui->title));
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

static GtkWidget *icon_button(const char *icon, const char *tooltip, GCallback callback, MkeyUi *ui)
{
    GtkWidget *button = gtk_button_new_from_icon_name(icon);
    gtk_button_set_has_frame(GTK_BUTTON(button), FALSE);
    gtk_widget_add_css_class(button, "flat");
    gtk_widget_set_size_request(button, 34, 34);
    gtk_widget_set_tooltip_text(button, tooltip);
    g_signal_connect(button, "clicked", callback, ui);
    return button;
}

static GtkWidget *menu_button(const char *icon, const char *tooltip, GtkPopover *popover)
{
    GtkWidget *button = gtk_menu_button_new();
    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(button), icon);
    gtk_menu_button_set_has_frame(GTK_MENU_BUTTON(button), FALSE);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(button), GTK_WIDGET(popover));
    gtk_widget_add_css_class(button, "flat");
    gtk_widget_set_tooltip_text(button, tooltip);
    return button;
}

static GtkWidget *create_toolbar(MkeyUi *ui)
{
    ui->toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_add_css_class(ui->toolbar, "toolbar");
    gtk_box_append(GTK_BOX(ui->toolbar), icon_button("list-add-symbolic", "Nuevo perfil (Ctrl+N)",
                                                     G_CALLBACK(new_clicked), ui));
    gtk_box_append(GTK_BOX(ui->toolbar), icon_button("edit-delete-symbolic", "Eliminar perfil",
                                                     G_CALLBACK(delete_clicked), ui));
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(ui->toolbar), spacer);
    ui->profiles_menu = GTK_POPOVER(gtk_popover_new());
    gtk_popover_set_position(ui->profiles_menu, GTK_POS_TOP);
    gtk_box_append(GTK_BOX(ui->toolbar),
                   menu_button("view-list-symbolic", "Perfiles de sonido", ui->profiles_menu));
    GtkWidget *emoji = gtk_emoji_chooser_new();
    g_signal_connect(emoji, "emoji-picked", G_CALLBACK(emoji_picked), ui);
    gtk_box_append(GTK_BOX(ui->toolbar), menu_button("face-smile-symbolic",
                                                     "Añadir emoji al título", GTK_POPOVER(emoji)));
    ui->appearance_menu = mkey_ui_appearance_menu(ui);
    gtk_box_append(GTK_BOX(ui->toolbar),
                   menu_button("emblem-system-symbolic", "Apariencia y preferencias (Ctrl+O)",
                               ui->appearance_menu));
    GtkWidget *handle = gtk_window_handle_new();
    gtk_window_handle_set_child(GTK_WINDOW_HANDLE(handle), ui->toolbar);
    return handle;
}

static GtkWidget *create_controls(MkeyUi *ui)
{
    GtkWidget *body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_add_css_class(body, "body");
    gtk_widget_set_vexpand(body, TRUE);
    mkey_widget_margins(body, 18, 16);
    gtk_box_append(GTK_BOX(body), mkey_row("Sonidos", mkey_ui_sound_pack_selector(ui)));
    ui->volume_label = GTK_LABEL(mkey_label(NULL, "volume"));
    gtk_box_append(GTK_BOX(body), mkey_row("Volumen", GTK_WIDGET(ui->volume_label)));
    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
    ui->volume_scale = GTK_RANGE(scale);
    gtk_box_append(GTK_BOX(body), scale);
    ui->keyboard_switch = GTK_SWITCH(gtk_switch_new());
    ui->mouse_switch = GTK_SWITCH(gtk_switch_new());
    gtk_box_append(GTK_BOX(body), mkey_row("Teclado", GTK_WIDGET(ui->keyboard_switch)));
    gtk_box_append(GTK_BOX(body), mkey_row("Mouse", GTK_WIDGET(ui->mouse_switch)));
    GtkWidget *tests = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 24);
    const char *names[] = {"⌨  Probar teclas", "↖  Probar clic"};
    for (int i = 0; i < 2; i++) {
        GtkWidget *button = gtk_button_new_with_label(names[i]);
        gtk_widget_add_css_class(button, "flat");
        gtk_widget_add_css_class(button, "test");
        g_object_set_data(G_OBJECT(button), "sample-id", GINT_TO_POINTER(i == 0 ? 0 : 3));
        g_signal_connect(button, "clicked", G_CALLBACK(preview_clicked), ui);
        gtk_box_append(GTK_BOX(tests), button);
    }
    gtk_box_append(GTK_BOX(body), tests);
    GtkWidget *test = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(test), "Escribe aquí para probar…");
    gtk_widget_add_css_class(test, "testfield");
    gtk_box_append(GTK_BOX(body), test);
    gtk_box_append(GTK_BOX(body),
                   mkey_label("Tus sonidos siguen al cerrar esta ventana.", "detail"));
    g_signal_connect(scale, "value-changed", G_CALLBACK(volume_changed), ui);
    g_signal_connect(ui->keyboard_switch, "notify::active", G_CALLBACK(device_changed), ui);
    g_signal_connect(ui->mouse_switch, "notify::active", G_CALLBACK(device_changed), ui);
    return body;
}

void mkey_ui_window_create(MkeyUi *ui)
{
    ui->css = gtk_css_provider_new();
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(ui->css),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    ui->window = GTK_WINDOW(gtk_application_window_new(ui->application));
    gtk_widget_add_css_class(GTK_WIDGET(ui->window), "mkey");
    gtk_window_set_default_size(ui->window, 430, 440);
    GtkWidget *hidden = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_visible(hidden, FALSE);
    gtk_window_set_titlebar(ui->window, hidden);
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *header = gtk_header_bar_new();
    gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(header), FALSE);
    ui->title = GTK_EDITABLE(gtk_editable_label_new(ui->appearance.title));
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), GTK_WIDGET(ui->title));
    GtkWidget *handle = gtk_window_handle_new();
    gtk_window_handle_set_child(GTK_WINDOW_HANDLE(handle), header);
    gtk_box_append(GTK_BOX(root), handle);
    gtk_box_append(GTK_BOX(root), create_controls(ui));
    gtk_box_append(GTK_BOX(root), create_toolbar(ui));
    gtk_window_set_child(ui->window, root);
    mkey_ui_refresh(ui);
    mkey_ui_save(ui);
    mkey_ui_profiles_rebuild(ui);
    g_signal_connect(ui->title, "notify::text", G_CALLBACK(title_changed), ui);
    g_signal_connect(ui->window, "close-request", G_CALLBACK(close_requested), ui);
    GtkEventController *keys = gtk_event_controller_key_new();
    g_signal_connect(keys, "key-pressed", G_CALLBACK(key_pressed), ui);
    gtk_widget_add_controller(GTK_WIDGET(ui->window), keys);
}
