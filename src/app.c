#include "app.h"
#include "webview.h"
#include "monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void on_window_destroy(GtkWidget *widget, gpointer user_data);

static const gchar *DEFAULT_CSS =
    "window.duckai-sidebar-window {\n"
    "    background-color: @theme_bg_color;\n"
    "    color: @theme_fg_color;\n"
    "}\n"
    ".duckai-sidebar-left {\n"
    "    border-right: 2px solid @theme_selected_bg_color;\n"
    "}\n"
    ".duckai-sidebar-right {\n"
    "    border-left: 2px solid @theme_selected_bg_color;\n"
    "}\n"
    ".duckai-header-bar {\n"
    "    background-color: alpha(@theme_base_color, 0.95);\n"
    "    border-bottom: 1px solid alpha(@theme_fg_color, 0.08);\n"
    "    padding: 2px 6px;\n"
    "}\n"
    ".duckai-header-title {\n"
    "    font-weight: 600;\n"
    "    font-size: 11px;\n"
    "    opacity: 0.85;\n"
    "    padding-left: 4px;\n"
    "}\n"
    ".duckai-header-btn {\n"
    "    padding: 2px 6px;\n"
    "    margin: 1px;\n"
    "    border-radius: 4px;\n"
    "    border: none;\n"
    "    background: transparent;\n"
    "    color: @theme_fg_color;\n"
    "    opacity: 0.75;\n"
    "}\n"
    ".duckai-header-btn:hover {\n"
    "    opacity: 1.0;\n"
    "    background-color: alpha(@theme_fg_color, 0.12);\n"
    "}\n";

static void setup_css(App *app) {
    app->css_provider = gtk_css_provider_new();
    GError *error = NULL;

    gchar *user_css = g_build_filename(g_get_user_config_dir(), "duckai-sidebar", "style.css", NULL);
    const gchar *sys_css_1 = "/usr/share/duckai-sidebar/duckai-sidebar.css";
    const gchar *sys_css_2 = "/usr/share/duckai-sidebar/style.css";

    if (g_file_test(user_css, G_FILE_TEST_EXISTS)) {
        gtk_css_provider_load_from_path(app->css_provider, user_css, &error);
    } else if (g_file_test(sys_css_1, G_FILE_TEST_EXISTS)) {
        gtk_css_provider_load_from_path(app->css_provider, sys_css_1, &error);
    } else if (g_file_test(sys_css_2, G_FILE_TEST_EXISTS)) {
        gtk_css_provider_load_from_path(app->css_provider, sys_css_2, &error);
    } else {
        gtk_css_provider_load_from_data(app->css_provider, DEFAULT_CSS, -1, &error);
    }

    if (error) {
        g_warning("CSS load error: %s", error->message);
        g_clear_error(&error);
    }

    GdkScreen *screen = gdk_screen_get_default();
    if (screen) {
        gtk_style_context_add_provider_for_screen(
            screen,
            GTK_STYLE_PROVIDER(app->css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
    g_free(user_css);
}

static GtkLayerShellLayer parse_layer(const gchar *layer_str) {
    if (!layer_str) return GTK_LAYER_SHELL_LAYER_TOP;
    if (g_ascii_strcasecmp(layer_str, "overlay") == 0) return GTK_LAYER_SHELL_LAYER_OVERLAY;
    if (g_ascii_strcasecmp(layer_str, "bottom") == 0) return GTK_LAYER_SHELL_LAYER_BOTTOM;
    if (g_ascii_strcasecmp(layer_str, "background") == 0) return GTK_LAYER_SHELL_LAYER_BACKGROUND;
    return GTK_LAYER_SHELL_LAYER_TOP;
}

static GtkLayerShellKeyboardMode parse_keyboard_mode(const gchar *km_str) {
    if (!km_str) return GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND;
    if (g_ascii_strcasecmp(km_str, "exclusive") == 0) return GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE;
    if (g_ascii_strcasecmp(km_str, "none") == 0) return GTK_LAYER_SHELL_KEYBOARD_MODE_NONE;
    return GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND;
}

static void apply_border_style(App *app) {
    if (!app || !app->window) return;

    if (!app->border_provider) {
        app->border_provider = gtk_css_provider_new();
        GtkStyleContext *ctx = gtk_widget_get_style_context(app->window);
        gtk_style_context_add_provider(
            ctx,
            GTK_STYLE_PROVIDER(app->border_provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER
        );
    }

    gint bw = app->cfg->border_width >= 0 ? app->cfg->border_width : 2;
    const gchar *bcolor = (app->cfg->border_color && *app->cfg->border_color)
                          ? app->cfg->border_color
                          : "@theme_selected_bg_color";
    gboolean is_right = (g_ascii_strcasecmp(app->current_edge, "right") == 0);
    gboolean mode_all = (app->cfg->border_mode && g_ascii_strcasecmp(app->cfg->border_mode, "all") == 0);

    gchar *border_css;
    if (mode_all) {
        border_css = g_strdup_printf(
            "window.duckai-sidebar-window {\n"
            "    border: %dpx solid %s;\n"
            "}\n", bw, bcolor);
    } else if (is_right) {
        border_css = g_strdup_printf(
            "window.duckai-sidebar-window {\n"
            "    border-left: %dpx solid %s;\n"
            "    border-right: none;\n"
            "    border-top: none;\n"
            "    border-bottom: none;\n"
            "}\n", bw, bcolor);
    } else {
        border_css = g_strdup_printf(
            "window.duckai-sidebar-window {\n"
            "    border-right: %dpx solid %s;\n"
            "    border-left: none;\n"
            "    border-top: none;\n"
            "    border-bottom: none;\n"
            "}\n", bw, bcolor);
    }

    gtk_css_provider_load_from_data(app->border_provider, border_css, -1, NULL);
    g_free(border_css);
}

static void apply_anchors_and_size(App *app) {
    GtkWindow *win = GTK_WINDOW(app->window);
    gboolean is_right = (g_ascii_strcasecmp(app->current_edge, "right") == 0);

    /* Stretch vertically between top and bottom */
    gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);

    if (is_right) {
        gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_LEFT, FALSE);
    } else {
        gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_RIGHT, FALSE);
    }

    /* Margins */
    gtk_layer_set_margin(win, GTK_LAYER_SHELL_EDGE_TOP, app->cfg->margin_top);
    gtk_layer_set_margin(win, GTK_LAYER_SHELL_EDGE_BOTTOM, app->cfg->margin_bottom);
    gtk_layer_set_margin(win, GTK_LAYER_SHELL_EDGE_LEFT, app->cfg->margin_left);
    gtk_layer_set_margin(win, GTK_LAYER_SHELL_EDGE_RIGHT, app->cfg->margin_right);

    /* Exclusive zone & width */
    if (app->cfg->exclusive_zone) {
        gtk_layer_set_exclusive_zone(win, app->current_width);
    } else {
        gtk_layer_set_exclusive_zone(win, 0);
    }
    gtk_widget_set_size_request(app->window, app->current_width, -1);

    /* Update CSS class for border placement */
    GtkStyleContext *ctx = gtk_widget_get_style_context(app->window);
    if (is_right) {
        gtk_style_context_remove_class(ctx, "duckai-sidebar-left");
        gtk_style_context_add_class(ctx, "duckai-sidebar-right");
    } else {
        gtk_style_context_remove_class(ctx, "duckai-sidebar-right");
        gtk_style_context_add_class(ctx, "duckai-sidebar-left");
    }

    apply_border_style(app);
}

static void on_btn_reload_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    App *app = (App *)user_data;
    webview_reload(app->webview, FALSE);
}

static void on_btn_newchat_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    App *app = (App *)user_data;
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(app->webview), app->cfg->url ? app->cfg->url : "https://duck.ai");
}

static void on_btn_external_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    App *app = (App *)user_data;
    const gchar *uri = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(app->webview));
    if (!uri) uri = "https://duck.ai";
    gtk_show_uri_on_window(NULL, uri, GDK_CURRENT_TIME, NULL);
}

static void on_btn_close_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    App *app = (App *)user_data;
    app_hide(app);
}

static GtkWidget *create_header_bar(App *app) {
    GtkWidget *bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_style_context_add_class(gtk_widget_get_style_context(bar), "duckai-header-bar");

    GtkWidget *title = gtk_label_new("Duck.ai");
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "duckai-header-title");
    gtk_box_pack_start(GTK_BOX(bar), title, FALSE, FALSE, 2);

    /* Spacer */
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(bar), spacer, TRUE, TRUE, 0);

    /* New Chat button */
    GtkWidget *btn_new = gtk_button_new_with_label("+ New");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_new), "duckai-header-btn");
    gtk_widget_set_tooltip_text(btn_new, "New chat");
    g_signal_connect(btn_new, "clicked", G_CALLBACK(on_btn_newchat_clicked), app);
    gtk_box_pack_start(GTK_BOX(bar), btn_new, FALSE, FALSE, 0);

    /* Reload button */
    GtkWidget *btn_reload = gtk_button_new_with_label("↺");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_reload), "duckai-header-btn");
    gtk_widget_set_tooltip_text(btn_reload, "Reload (Ctrl+R / F5)");
    g_signal_connect(btn_reload, "clicked", G_CALLBACK(on_btn_reload_clicked), app);
    gtk_box_pack_start(GTK_BOX(bar), btn_reload, FALSE, FALSE, 0);

    /* Open in browser button */
    GtkWidget *btn_ext = gtk_button_new_with_label("↗");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_ext), "duckai-header-btn");
    gtk_widget_set_tooltip_text(btn_ext, "Open in default browser");
    g_signal_connect(btn_ext, "clicked", G_CALLBACK(on_btn_external_clicked), app);
    gtk_box_pack_start(GTK_BOX(bar), btn_ext, FALSE, FALSE, 0);

    /* Close / Hide button */
    GtkWidget *btn_close = gtk_button_new_with_label("✕");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_close), "duckai-header-btn");
    gtk_widget_set_tooltip_text(btn_close, "Hide sidebar (Esc)");
    g_signal_connect(btn_close, "clicked", G_CALLBACK(on_btn_close_clicked), app);
    gtk_box_pack_start(GTK_BOX(bar), btn_close, FALSE, FALSE, 0);

    return bar;
}

App *app_create(SidebarConfig *cfg) {
    if (!gtk_layer_is_supported()) {
        g_printerr("Error: Wayland layer-shell protocol is not supported by this compositor!\n");
        return NULL;
    }

    App *app = g_new0(App, 1);
    app->cfg = cfg;
    app->is_visible = FALSE;
    app->current_width = cfg->width > 0 ? cfg->width : 420;
    app->current_edge = g_strdup(cfg->edge ? cfg->edge : "right");
    app->current_monitor_name = g_strdup(cfg->monitor ? cfg->monitor : "active");

    /* Create window without decorations */
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(app->window), FALSE);
    gtk_widget_set_app_paintable(app->window, TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->window), "duckai-sidebar-window");

    /* Init Layer Shell */
    gtk_layer_init_for_window(GTK_WINDOW(app->window));
    gtk_layer_set_namespace(GTK_WINDOW(app->window), "duckai-sidebar");
    gtk_layer_set_layer(GTK_WINDOW(app->window), parse_layer(cfg->layer));
    gtk_layer_set_keyboard_mode(GTK_WINDOW(app->window), parse_keyboard_mode(cfg->keyboard_mode));

    /* Resolve and set monitor */
    GdkDisplay *disp = gdk_display_get_default();
    GdkMonitor *mon = monitor_resolve(disp, app->current_monitor_name);
    if (mon) {
        gtk_layer_set_monitor(GTK_WINDOW(app->window), mon);
    }

    /* Anchors, margins, exclusive zone, size */
    apply_anchors_and_size(app);

    /* Setup container box */
    app->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app->window), app->main_box);

    /* Header bar */
    app->header_bar = create_header_bar(app);
    gtk_box_pack_start(GTK_BOX(app->main_box), app->header_bar, FALSE, FALSE, 0);
    if (!cfg->show_header) {
        gtk_widget_hide(app->header_bar);
    }

    /* Webview */
    app->webview = webview_create(app, cfg);
    gtk_box_pack_start(GTK_BOX(app->main_box), app->webview, TRUE, TRUE, 0);

    /* Setup theme styles */
    setup_css(app);

    /* Close handling */
    g_signal_connect(app->window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    return app;
}

void app_destroy(App *app) {
    if (!app) return;
    if (app->window && GTK_IS_WIDGET(app->window)) {
        gtk_widget_destroy(app->window);
        app->window = NULL;
    }
    if (app->border_provider) {
        g_object_unref(app->border_provider);
        app->border_provider = NULL;
    }
    if (app->css_provider) {
        GdkScreen *screen = gdk_screen_get_default();
        if (screen) {
            gtk_style_context_remove_provider_for_screen(screen, GTK_STYLE_PROVIDER(app->css_provider));
        }
        g_object_unref(app->css_provider);
        app->css_provider = NULL;
    }
    g_free(app->current_edge);
    g_free(app->current_monitor_name);
    g_free(app);
}

void app_show(App *app) {
    if (!app || !app->window) return;

    /* If monitor spec is "active", resolve current active monitor before showing */
    if (g_strcmp0(app->current_monitor_name, "active") == 0 || app->current_monitor_name == NULL) {
        GdkDisplay *disp = gdk_display_get_default();
        GdkMonitor *active_mon = monitor_resolve(disp, "active");
        if (active_mon) {
            gtk_layer_set_monitor(GTK_WINDOW(app->window), active_mon);
        }
    }

    apply_anchors_and_size(app);

    /* Show window and components without header flicker */
    gtk_widget_show(app->window);
    gtk_widget_show(app->main_box);
    gtk_widget_show(app->webview);
    if (app->cfg->show_header) {
        gtk_widget_show_all(app->header_bar);
    } else {
        gtk_widget_hide(app->header_bar);
    }

    gtk_widget_grab_focus(app->webview);
    app->is_visible = TRUE;
}

void app_hide(App *app) {
    if (!app || !app->window) return;
    gtk_widget_hide(app->window);
    app->is_visible = FALSE;
}

void app_toggle(App *app) {
    if (!app) return;
    if (app->is_visible) {
        app_hide(app);
    } else {
        app_show(app);
    }
}

void app_reload(App *app, gboolean bypass_cache) {
    if (!app) return;
    webview_reload(app->webview, bypass_cache);
}

void app_set_width(App *app, gint width) {
    if (!app || width < 150 || width > 3840) return;
    app->current_width = width;
    apply_anchors_and_size(app);
}

void app_set_edge(App *app, const gchar *edge) {
    if (!app || !edge) return;
    if (g_ascii_strcasecmp(edge, "left") != 0 && g_ascii_strcasecmp(edge, "right") != 0) return;
    g_free(app->current_edge);
    app->current_edge = g_strdup(edge);
    apply_anchors_and_size(app);
}

void app_set_monitor(App *app, const gchar *monitor_spec) {
    if (!app || !monitor_spec) return;
    g_free(app->current_monitor_name);
    app->current_monitor_name = g_strdup(monitor_spec);
    GdkDisplay *disp = gdk_display_get_default();
    GdkMonitor *mon = monitor_resolve(disp, monitor_spec);
    if (mon) {
        gtk_layer_set_monitor(GTK_WINDOW(app->window), mon);
    }
}

void app_set_border(App *app, gint width, const gchar *color) {
    if (!app) return;
    if (width >= 0) app->cfg->border_width = width;
    if (color && *color) {
        g_free(app->cfg->border_color);
        app->cfg->border_color = g_strdup(color);
    }
    apply_border_style(app);
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (gtk_main_level() > 0) {
        gtk_main_quit();
    }
}

void app_quit(App *app) {
    (void)app;
    if (gtk_main_level() > 0) {
        gtk_main_quit();
    }
}

gboolean app_is_visible(const App *app) {
    return app ? app->is_visible : FALSE;
}

const gchar *app_get_status_string(App *app) {
    static gchar status[256];
    gchar *active_mon = monitor_get_active_name();
    snprintf(status, sizeof(status), "STATUS visible=%d width=%d edge=%s monitor=%s",
             app->is_visible ? 1 : 0,
             app->current_width,
             app->current_edge,
             active_mon);
    g_free(active_mon);
    return status;
}
