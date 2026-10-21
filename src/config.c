#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SidebarConfig *config_new(void) {
    SidebarConfig *cfg = g_new0(SidebarConfig, 1);
    cfg->url = g_strdup("https://duck.ai");
    cfg->width = 420;
    cfg->edge = g_strdup("right");
    cfg->layer = g_strdup("top");
    cfg->monitor = g_strdup("active");
    cfg->exclusive_zone = TRUE;
    cfg->keyboard_mode = g_strdup("on-demand");
    cfg->show_header = FALSE;
    cfg->close_on_escape = TRUE;

    cfg->margin_top = 0;
    cfg->margin_bottom = 0;
    cfg->margin_left = 0;
    cfg->margin_right = 0;

    cfg->border_width = 2;
    cfg->border_color = NULL;
    cfg->border_mode = g_strdup("inner");

    cfg->enable_smooth_scrolling = TRUE;
    cfg->enable_webgl = TRUE;
    cfg->enable_clipboard = TRUE;
    cfg->user_agent = NULL;

    cfg->config_path = NULL;
    return cfg;
}

void config_free(SidebarConfig *cfg) {
    if (!cfg) return;
    g_free(cfg->url);
    g_free(cfg->edge);
    g_free(cfg->layer);
    g_free(cfg->monitor);
    g_free(cfg->keyboard_mode);
    g_free(cfg->border_color);
    g_free(cfg->border_mode);
    g_free(cfg->user_agent);
    g_free(cfg->config_path);
    g_free(cfg);
}

gboolean config_load_file(SidebarConfig *cfg, const gchar *path) {
    if (!path || !g_file_test(path, G_FILE_TEST_EXISTS)) {
        return FALSE;
    }

    GError *error = NULL;
    GKeyFile *kf = g_key_file_new();

    if (!g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, &error)) {
        g_warning("Failed to load config file '%s': %s", path, error ? error->message : "unknown error");
        g_clear_error(&error);
        g_key_file_free(kf);
        return FALSE;
    }

    g_free(cfg->config_path);
    cfg->config_path = g_strdup(path);

    /* [general] group */
    if (g_key_file_has_group(kf, "general")) {
        gchar *url = g_key_file_get_string(kf, "general", "url", NULL);
        if (url && *url) {
            g_free(cfg->url);
            cfg->url = g_strstrip(url);
        } else {
            g_free(url);
        }

        if (g_key_file_has_key(kf, "general", "width", NULL)) {
            gint w = g_key_file_get_integer(kf, "general", "width", NULL);
            if (w >= 150 && w <= 3840) {
                cfg->width = w;
            }
        }

        gchar *edge = g_key_file_get_string(kf, "general", "edge", NULL);
        if (edge && *edge) {
            g_strstrip(edge);
            if (g_ascii_strcasecmp(edge, "left") == 0 || g_ascii_strcasecmp(edge, "right") == 0) {
                g_free(cfg->edge);
                cfg->edge = edge;
            } else {
                g_free(edge);
            }
        } else {
            g_free(edge);
        }

        gchar *layer = g_key_file_get_string(kf, "general", "layer", NULL);
        if (layer && *layer) {
            g_free(cfg->layer);
            cfg->layer = g_strstrip(layer);
        } else {
            g_free(layer);
        }

        gchar *monitor = g_key_file_get_string(kf, "general", "monitor", NULL);
        if (monitor && *monitor) {
            g_free(cfg->monitor);
            cfg->monitor = g_strstrip(monitor);
        } else {
            g_free(monitor);
        }

        if (g_key_file_has_key(kf, "general", "exclusive_zone", NULL)) {
            cfg->exclusive_zone = g_key_file_get_boolean(kf, "general", "exclusive_zone", NULL);
        }

        gchar *km = g_key_file_get_string(kf, "general", "keyboard_mode", NULL);
        if (km && *km) {
            g_free(cfg->keyboard_mode);
            cfg->keyboard_mode = g_strstrip(km);
        } else {
            g_free(km);
        }

        if (g_key_file_has_key(kf, "general", "show_header", NULL)) {
            cfg->show_header = g_key_file_get_boolean(kf, "general", "show_header", NULL);
        }

        if (g_key_file_has_key(kf, "general", "close_on_escape", NULL)) {
            cfg->close_on_escape = g_key_file_get_boolean(kf, "general", "close_on_escape", NULL);
        }
    }

    /* [margins] group */
    if (g_key_file_has_group(kf, "margins")) {
        if (g_key_file_has_key(kf, "margins", "top", NULL))
            cfg->margin_top = g_key_file_get_integer(kf, "margins", "top", NULL);
        if (g_key_file_has_key(kf, "margins", "bottom", NULL))
            cfg->margin_bottom = g_key_file_get_integer(kf, "margins", "bottom", NULL);
        if (g_key_file_has_key(kf, "margins", "left", NULL))
            cfg->margin_left = g_key_file_get_integer(kf, "margins", "left", NULL);
        if (g_key_file_has_key(kf, "margins", "right", NULL))
            cfg->margin_right = g_key_file_get_integer(kf, "margins", "right", NULL);
    }

    /* [appearance] group */
    if (g_key_file_has_group(kf, "appearance")) {
        if (g_key_file_has_key(kf, "appearance", "border_width", NULL)) {
            gint bw = g_key_file_get_integer(kf, "appearance", "border_width", NULL);
            if (bw >= 0 && bw <= 32) {
                cfg->border_width = bw;
            }
        }
        gchar *bc = g_key_file_get_string(kf, "appearance", "border_color", NULL);
        if (bc && *bc) {
            g_free(cfg->border_color);
            cfg->border_color = g_strstrip(bc);
        } else {
            g_free(bc);
        }
        gchar *bm = g_key_file_get_string(kf, "appearance", "border_mode", NULL);
        if (bm && *bm) {
            g_free(cfg->border_mode);
            cfg->border_mode = g_strstrip(bm);
        } else {
            g_free(bm);
        }
    }

    /* [webview] group */
    if (g_key_file_has_group(kf, "webview")) {
        if (g_key_file_has_key(kf, "webview", "enable_smooth_scrolling", NULL))
            cfg->enable_smooth_scrolling = g_key_file_get_boolean(kf, "webview", "enable_smooth_scrolling", NULL);
        if (g_key_file_has_key(kf, "webview", "enable_webgl", NULL))
            cfg->enable_webgl = g_key_file_get_boolean(kf, "webview", "enable_webgl", NULL);
        if (g_key_file_has_key(kf, "webview", "enable_clipboard", NULL))
            cfg->enable_clipboard = g_key_file_get_boolean(kf, "webview", "enable_clipboard", NULL);

        gchar *ua = g_key_file_get_string(kf, "webview", "user_agent", NULL);
        if (ua && *ua) {
            g_free(cfg->user_agent);
            cfg->user_agent = g_strstrip(ua);
        } else {
            g_free(ua);
        }
    }

    g_key_file_free(kf);
    return TRUE;
}

gboolean config_load_default(SidebarConfig *cfg) {
    const gchar *config_home = g_get_user_config_dir();
    gchar *default_path = g_build_filename(config_home, "duckai-sidebar", "config.ini", NULL);
    gboolean loaded = config_load_file(cfg, default_path);
    g_free(default_path);
    return loaded;
}

void config_print(const SidebarConfig *cfg) {
    if (!cfg) return;
    g_print("duckai-sidebar configuration:\n");
    g_print("  [general]\n");
    g_print("    url:             %s\n", cfg->url ? cfg->url : "(default)");
    g_print("    width:           %d px\n", cfg->width);
    g_print("    edge:            %s\n", cfg->edge);
    g_print("    layer:           %s\n", cfg->layer);
    g_print("    monitor:         %s\n", cfg->monitor);
    g_print("    exclusive_zone:  %s\n", cfg->exclusive_zone ? "true" : "false");
    g_print("    keyboard_mode:   %s\n", cfg->keyboard_mode);
    g_print("    show_header:     %s\n", cfg->show_header ? "true" : "false");
    g_print("    close_on_escape: %s\n", cfg->close_on_escape ? "true" : "false");
    g_print("  [appearance]\n");
    g_print("    border_width:    %d px\n", cfg->border_width);
    g_print("    border_color:    %s\n", (cfg->border_color && *cfg->border_color) ? cfg->border_color : "(theme accent)");
    g_print("    border_mode:     %s\n", cfg->border_mode ? cfg->border_mode : "inner");
    g_print("  [margins]\n");
    g_print("    top/bottom/left/right: %d / %d / %d / %d px\n",
            cfg->margin_top, cfg->margin_bottom, cfg->margin_left, cfg->margin_right);
    g_print("  [webview]\n");
    g_print("    smooth_scrolling:%s\n", cfg->enable_smooth_scrolling ? "true" : "false");
    g_print("    webgl:           %s\n", cfg->enable_webgl ? "true" : "false");
    g_print("    clipboard:       %s\n", cfg->enable_clipboard ? "true" : "false");
    g_print("    user_agent:      %s\n", (cfg->user_agent && *cfg->user_agent) ? cfg->user_agent : "(default)");
    if (cfg->config_path) {
        g_print("  loaded from:       %s\n", cfg->config_path);
    }
}
