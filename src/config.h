#ifndef CONFIG_H
#define CONFIG_H

#include <glib.h>

typedef struct {
    gchar *url;
    gint width;
    gchar *edge;
    gchar *layer;
    gchar *monitor;
    gboolean exclusive_zone;
    gchar *keyboard_mode;
    gboolean show_header;
    gboolean close_on_escape;

    gint margin_top;
    gint margin_bottom;
    gint margin_left;
    gint margin_right;

    gint border_width;
    gchar *border_color;
    gchar *border_mode;

    gboolean enable_smooth_scrolling;
    gboolean enable_webgl;
    gboolean enable_clipboard;
    gchar *user_agent;

    gchar *config_path;
} SidebarConfig;

SidebarConfig *config_new(void);
void config_free(SidebarConfig *cfg);
gboolean config_load_file(SidebarConfig *cfg, const gchar *path);
gboolean config_load_default(SidebarConfig *cfg);
void config_print(const SidebarConfig *cfg);

#endif /* CONFIG_H */
