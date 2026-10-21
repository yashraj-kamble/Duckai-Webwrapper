#include "monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static gboolean get_hyprctl_monitor_geom(const gchar *spec, gint *out_x, gint *out_y, gint *out_w, gint *out_h) {
    gchar *stdout_buf = NULL;
    gint exit_status = 0;
    GError *error = NULL;
    gchar *argv[] = {"hyprctl", "monitors", "-j", NULL};

    if (!g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &stdout_buf, NULL, &exit_status, &error)) {
        g_clear_error(&error);
        return FALSE;
    }
    if (exit_status != 0 || !stdout_buf) {
        g_free(stdout_buf);
        return FALSE;
    }

    gboolean looking_for_active = (g_strcmp0(spec, "active") == 0 || spec == NULL || *spec == '\0');
    gboolean found = FALSE;

    /* Scan JSON array by matching balanced braces for each top-level object */
    const gchar *p = stdout_buf;
    while (*p) {
        /* Find start of next object */
        while (*p && *p != '{') p++;
        if (!*p) break;

        const gchar *obj_start = p;
        int depth = 0;
        gboolean in_string = FALSE;
        gboolean escape = FALSE;

        while (*p) {
            if (escape) {
                escape = FALSE;
            } else if (*p == '\\') {
                escape = TRUE;
            } else if (*p == '"') {
                in_string = !in_string;
            } else if (!in_string) {
                if (*p == '{') {
                    depth++;
                } else if (*p == '}') {
                    depth--;
                    if (depth == 0) {
                        p++; /* include closing brace */
                        break;
                    }
                }
            }
            p++;
        }

        gchar *obj = g_strndup(obj_start, p - obj_start);
        gboolean match = FALSE;

        if (looking_for_active) {
            if (strstr(obj, "\"focused\": true") != NULL || strstr(obj, "\"focused\":true") != NULL) {
                match = TRUE;
            }
        } else {
            gchar *name_needle = g_strdup_printf("\"name\": \"%s\"", spec);
            gchar *id_needle = g_strdup_printf("\"id\": %s", spec);
            if (strstr(obj, name_needle) != NULL || strstr(obj, id_needle) != NULL) {
                match = TRUE;
            }
            g_free(name_needle);
            g_free(id_needle);
        }

        if (match) {
            gint x = 0, y = 0, w = 0, h = 0;
            const gchar *field;

            if ((field = strstr(obj, "\"x\":")) != NULL) x = atoi(field + 4);
            if ((field = strstr(obj, "\"y\":")) != NULL) y = atoi(field + 4);
            if ((field = strstr(obj, "\"width\":")) != NULL) w = atoi(field + 8);
            if ((field = strstr(obj, "\"height\":")) != NULL) h = atoi(field + 9);

            if (w > 0 && h > 0) {
                *out_x = x;
                *out_y = y;
                *out_w = w;
                *out_h = h;
                found = TRUE;
                g_free(obj);
                break;
            }
        }

        g_free(obj);
    }

    g_free(stdout_buf);
    return found;
}

GdkMonitor *monitor_resolve(GdkDisplay *display, const gchar *monitor_spec) {
    if (!display) {
        display = gdk_display_get_default();
        if (!display) return NULL;
    }

    gint n_monitors = gdk_display_get_n_monitors(display);
    if (n_monitors <= 0) return NULL;
    if (n_monitors == 1) return gdk_display_get_monitor(display, 0);

    /* 1. Try hyprctl */
    gint hx = 0, hy = 0, hw = 0, hh = 0;
    if (get_hyprctl_monitor_geom(monitor_spec, &hx, &hy, &hw, &hh)) {
        for (gint i = 0; i < n_monitors; i++) {
            GdkMonitor *m = gdk_display_get_monitor(display, i);
            GdkRectangle geom;
            gdk_monitor_get_geometry(m, &geom);

            /* Matches origin (x, y) coordinates */
            if (geom.x == hx && geom.y == hy) {
                return m;
            }
        }
    }

    /* 2. Fallback to pointer position if "active" */
    if (g_strcmp0(monitor_spec, "active") == 0 || monitor_spec == NULL) {
        GdkSeat *seat = gdk_display_get_default_seat(display);
        if (seat) {
            GdkDevice *pointer = gdk_seat_get_pointer(seat);
            if (pointer) {
                gint px = 0, py = 0;
                gdk_device_get_position(pointer, NULL, &px, &py);
                GdkMonitor *m = gdk_display_get_monitor_at_point(display, px, py);
                if (m) return m;
            }
        }
    }

    /* 3. Primary monitor fallback */
    GdkMonitor *primary = gdk_display_get_primary_monitor(display);
    if (primary) return primary;

    /* 4. Default first monitor */
    return gdk_display_get_monitor(display, 0);
}

gchar *monitor_get_active_name(void) {
    gchar *stdout_buf = NULL;
    gint exit_status = 0;
    GError *error = NULL;
    gchar *argv[] = {"hyprctl", "activeworkspace", "-j", NULL};

    if (!g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &stdout_buf, NULL, &exit_status, &error)) {
        g_clear_error(&error);
        return g_strdup("default");
    }
    if (exit_status != 0 || !stdout_buf) {
        g_free(stdout_buf);
        return g_strdup("default");
    }

    const gchar *p = strstr(stdout_buf, "\"monitor\": \"");
    if (p) {
        p += 12;
        const gchar *end = strchr(p, '"');
        if (end) {
            gchar *name = g_strndup(p, end - p);
            g_free(stdout_buf);
            return name;
        }
    }

    g_free(stdout_buf);
    return g_strdup("default");
}
