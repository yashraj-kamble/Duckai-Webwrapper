#ifndef APP_H
#define APP_H

#include <gtk/gtk.h>
#include <gtk-layer-shell.h>
#include "config.h"

typedef struct App {
    GtkWidget *window;
    GtkWidget *webview;
    GtkWidget *header_bar;
    GtkWidget *main_box;

    SidebarConfig *cfg;
    gboolean is_visible;
    gint current_width;
    gchar *current_edge;
    gchar *current_monitor_name;

    GtkCssProvider *css_provider;
    GtkCssProvider *border_provider;
} App;

App *app_create(SidebarConfig *cfg);
void app_destroy(App *app);

void app_show(App *app);
void app_hide(App *app);
void app_toggle(App *app);
void app_reload(App *app, gboolean bypass_cache);

void app_set_width(App *app, gint width);
void app_set_edge(App *app, const gchar *edge);
void app_set_monitor(App *app, const gchar *monitor_spec);
void app_set_border(App *app, gint width, const gchar *color);
void app_quit(App *app);

gboolean app_is_visible(const App *app);
const gchar *app_get_status_string(App *app);

#endif /* APP_H */
