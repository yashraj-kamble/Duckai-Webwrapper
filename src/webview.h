#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include "config.h"

typedef struct App App;

GtkWidget *webview_create(App *app, const SidebarConfig *cfg);
void webview_reload(GtkWidget *webview, gboolean bypass_cache);
void webview_zoom_in(GtkWidget *webview);
void webview_zoom_out(GtkWidget *webview);
void webview_zoom_reset(GtkWidget *webview);

#endif /* WEBVIEW_H */
