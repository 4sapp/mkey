/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui.h"

static gboolean save_screenshot(gpointer data)
{
    MkeyUi *ui = data;
    GtkWidget *widget = GTK_WIDGET(ui->window);
    g_autoptr(GdkPaintable) paintable = gtk_widget_paintable_new(widget);
    GtkSnapshot *snapshot = gtk_snapshot_new();
    gdk_paintable_snapshot(paintable, GDK_SNAPSHOT(snapshot), gtk_widget_get_width(widget),
                           gtk_widget_get_height(widget));
    GskRenderNode *node = gtk_snapshot_free_to_node(snapshot);
    if (node) {
        GskRenderer *renderer = gtk_native_get_renderer(GTK_NATIVE(ui->window));
        g_autoptr(GdkTexture) texture = gsk_renderer_render_texture(renderer, node, NULL);
        gdk_texture_save_to_png(texture, ui->screenshot);
        gsk_render_node_unref(node);
    }
    g_application_quit(G_APPLICATION(ui->application));
    return G_SOURCE_REMOVE;
}

static gboolean run_smoke_test(gpointer data)
{
    MkeyUi *ui = data;
    gtk_range_set_value(ui->volume_scale, 82);
    g_assert_cmpint(ui->settings.volume, ==, 82);
    gtk_switch_set_active(ui->keyboard_switch, FALSE);
    gtk_switch_set_active(ui->mouse_switch, FALSE);
    g_assert_false(ui->settings.keyboard_enabled);
    g_assert_false(ui->settings.mouse_enabled);
    ui->settings.theme = 7;
    mkey_ui_save(ui);
    mkey_ui_profile_new(ui);
    g_autofree char *profile = g_strdup(ui->appearance.profile);
    g_assert_true(mkey_ui_profile_load(ui, "default"));
    g_assert_true(mkey_ui_profile_load(ui, profile));
    g_assert_cmpint(ui->settings.theme, ==, 7);
    mkey_ui_profile_delete(ui);
    g_assert_cmpint(ui->settings.volume, ==, 70);
    mkey_ui_profile_restore(ui);
    g_assert_cmpint(ui->settings.volume, ==, 82);
    g_assert_false(ui->settings.keyboard_enabled);
    gtk_switch_set_active(ui->monospace_switch, TRUE);
    g_assert_true(ui->appearance.monospace);
    MkeySettings saved;
    g_assert_true(mkey_settings_load(ui->paths.settings_file, &saved, NULL));
    g_assert_cmpint(saved.volume, ==, 82);
    g_print("PASS: controls, persistence, profiles, deletion/restore, appearance\n");
    g_application_quit(G_APPLICATION(ui->application));
    return G_SOURCE_REMOVE;
}

static void activate(GtkApplication *application, gpointer data)
{
    MkeyUi *ui = data;
    ui->application = application;
    if (!ui->window) {
        mkey_ui_window_create(ui);
        if (ui->screenshot) {
            g_timeout_add(800, save_screenshot, ui);
        } else if (ui->smoke_test) {
            g_timeout_add(400, run_smoke_test, ui);
        }
    }
    gtk_window_present(ui->window);
}

int main(int argc, char **argv)
{
    MkeyUi ui = {0};
    g_autoptr(GError) error = NULL;
    mkey_paths_init(&ui.paths, g_getenv("MKEY_DATA_DIR"));
    if (!mkey_paths_create(&ui.paths, &error)) {
        g_printerr("%s\n", error->message);
        mkey_paths_clear(&ui.paths);
        return EXIT_FAILURE;
    }
    mkey_ui_load(&ui);
    if (argc == 3 && g_str_equal(argv[1], "--screenshot")) {
        ui.screenshot = argv[2];
        argc = 1;
    } else if (argc == 2 && g_str_equal(argv[1], "--smoke-test")) {
        ui.smoke_test = TRUE;
        argc = 1;
    }
    g_autoptr(GtkApplication) application =
        gtk_application_new("local.davo.mkey", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(application, "activate", G_CALLBACK(activate), &ui);
    int result = g_application_run(G_APPLICATION(application), argc, argv);
    g_clear_object(&ui.css);
    g_free(ui.appearance.title);
    g_free(ui.appearance.profile);
    mkey_paths_clear(&ui.paths);
    return result;
}
