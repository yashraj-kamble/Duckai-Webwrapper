#ifndef MONITOR_H
#define MONITOR_H

#include <gtk/gtk.h>

GdkMonitor *monitor_resolve(GdkDisplay *display, const gchar *monitor_spec);
gchar *monitor_get_active_name(void);

#endif /* MONITOR_H */
