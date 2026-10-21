#include "webview.h"
#include "app.h"
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static gboolean is_internal_url(const gchar *uri) {
    if (!uri) return FALSE;
    if (strstr(uri, "duck.ai") != NULL ||
        strstr(uri, "duckduckgo.com") != NULL ||
        g_str_has_prefix(uri, "about:") ||
        g_str_has_prefix(uri, "blob:")) {
        return TRUE;
    }
    return FALSE;
}

static gboolean on_decide_policy(WebKitWebView *web_view,
                                 WebKitPolicyDecision *decision,
                                 WebKitPolicyDecisionType type,
                                 gpointer user_data) {
    (void)web_view;
    (void)user_data;
    if (type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) {
        WebKitNavigationPolicyDecision *nav_dec = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
        WebKitNavigationAction *action = webkit_navigation_policy_decision_get_navigation_action(nav_dec);
        WebKitURIRequest *req = webkit_navigation_action_get_request(action);
        const gchar *uri = req ? webkit_uri_request_get_uri(req) : NULL;
        WebKitNavigationType nav_type = webkit_navigation_action_get_navigation_type(action);

        if (nav_type == WEBKIT_NAVIGATION_TYPE_LINK_CLICKED && uri) {
            if (!is_internal_url(uri)) {
                /* Open external link in system default browser */
                GError *err = NULL;
                if (!gtk_show_uri_on_window(NULL, uri, GDK_CURRENT_TIME, &err)) {
                    g_warning("Failed to open external URI '%s': %s", uri, err ? err->message : "unknown");
                    g_clear_error(&err);
                }
                webkit_policy_decision_ignore(decision);
                return TRUE;
            }
        }
    }
    return FALSE;
}

static GtkWidget *on_create_web_view(WebKitWebView *web_view,
                                     WebKitNavigationAction *action,
                                     gpointer user_data) {
    (void)user_data;
    WebKitURIRequest *req = webkit_navigation_action_get_request(action);
    const gchar *uri = req ? webkit_uri_request_get_uri(req) : NULL;

    if (uri && !is_internal_url(uri)) {
        gtk_show_uri_on_window(NULL, uri, GDK_CURRENT_TIME, NULL);
        return NULL;
    }

    /* Create popup window sharing the same WebContext for login / auth */
    GtkWidget *popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(popup), "Duck.ai - Sign in");
    gtk_window_set_default_size(GTK_WINDOW(popup), 640, 720);

    WebKitWebContext *ctx = webkit_web_view_get_context(web_view);
    GtkWidget *popup_view = webkit_web_view_new_with_context(ctx);

    /* Inherit settings */
    WebKitSettings *settings = webkit_web_view_get_settings(web_view);
    webkit_web_view_set_settings(WEBKIT_WEB_VIEW(popup_view), settings);

    g_signal_connect_swapped(popup_view, "close", G_CALLBACK(gtk_widget_destroy), popup);
    gtk_container_add(GTK_CONTAINER(popup), popup_view);
    gtk_widget_show_all(popup);

    return popup_view;
}

static gboolean on_permission_request(WebKitWebView *web_view,
                                      WebKitPermissionRequest *request,
                                      gpointer user_data) {
    (void)web_view;
    (void)user_data;
    webkit_permission_request_allow(request);
    return TRUE;
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    (void)widget;
    App *app = (App *)user_data;
    WebKitWebView *wv = WEBKIT_WEB_VIEW(app->webview);

    guint state = event->state & gtk_accelerator_get_default_mod_mask();

    /* Escape: hide sidebar */
    if (app->cfg->close_on_escape && event->keyval == GDK_KEY_Escape && state == 0) {
        app_hide(app);
        return TRUE;
    }

    /* Ctrl + R: reload (Shift for cache bypass) */
    if ((state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_r || event->keyval == GDK_KEY_R)) {
        gboolean bypass = (state & GDK_SHIFT_MASK) != 0;
        webview_reload(app->webview, bypass);
        return TRUE;
    }

    /* F5: reload */
    if (event->keyval == GDK_KEY_F5) {
        gboolean bypass = (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) != 0;
        webview_reload(app->webview, bypass);
        return TRUE;
    }

    /* Ctrl + Plus / Equal / KP_Add: Zoom In */
    if ((state & GDK_CONTROL_MASK) &&
        (event->keyval == GDK_KEY_plus || event->keyval == GDK_KEY_equal || event->keyval == GDK_KEY_KP_Add)) {
        webview_zoom_in(app->webview);
        return TRUE;
    }

    /* Ctrl + Minus / KP_Subtract: Zoom Out */
    if ((state & GDK_CONTROL_MASK) &&
        (event->keyval == GDK_KEY_minus || event->keyval == GDK_KEY_KP_Subtract)) {
        webview_zoom_out(app->webview);
        return TRUE;
    }

    /* Ctrl + 0 / KP_0: Zoom Reset */
    if ((state & GDK_CONTROL_MASK) &&
        (event->keyval == GDK_KEY_0 || event->keyval == GDK_KEY_KP_0)) {
        webview_zoom_reset(app->webview);
        return TRUE;
    }

    /* Alt + Left: Back */
    if ((state & GDK_MOD1_MASK) && event->keyval == GDK_KEY_Left) {
        if (webkit_web_view_can_go_back(wv)) {
            webkit_web_view_go_back(wv);
            return TRUE;
        }
    }

    /* Alt + Right: Forward */
    if ((state & GDK_MOD1_MASK) && event->keyval == GDK_KEY_Right) {
        if (webkit_web_view_can_go_forward(wv)) {
            webkit_web_view_go_forward(wv);
            return TRUE;
        }
    }

    return FALSE;
}

GtkWidget *webview_create(App *app, const SidebarConfig *cfg) {
    /* Setup persistent data paths */
    const gchar *data_home = g_get_user_data_dir();
    const gchar *cache_home = g_get_user_cache_dir();

    gchar *data_dir = g_build_filename(data_home, "duckai-sidebar", "webdata", NULL);
    gchar *cache_dir = g_build_filename(cache_home, "duckai-sidebar", "cache", NULL);
    gchar *cookie_file = g_build_filename(data_home, "duckai-sidebar", "cookies.sqlite", NULL);

    g_mkdir_with_parents(data_dir, 0700);
    g_mkdir_with_parents(cache_dir, 0700);

    WebKitWebsiteDataManager *data_manager = webkit_website_data_manager_new(
        "base-data-directory", data_dir,
        "base-cache-directory", cache_dir,
        NULL
    );

    WebKitWebContext *context = webkit_web_context_new_with_website_data_manager(data_manager);
    g_object_unref(data_manager);

    /* Optimize cache model for web applications */
    webkit_web_context_set_cache_model(context, WEBKIT_CACHE_MODEL_WEB_BROWSER);

    WebKitCookieManager *cookie_manager = webkit_web_context_get_cookie_manager(context);
    webkit_cookie_manager_set_persistent_storage(
        cookie_manager,
        cookie_file,
        WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE
    );
    webkit_cookie_manager_set_accept_policy(
        cookie_manager,
        WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS
    );

    g_free(data_dir);
    g_free(cache_dir);
    g_free(cookie_file);

    /* WebKit Settings with GPU Acceleration & Performance Optimizations */
    WebKitSettings *settings = webkit_settings_new();
    webkit_settings_set_enable_smooth_scrolling(settings, cfg->enable_smooth_scrolling);
    webkit_settings_set_enable_webgl(settings, cfg->enable_webgl);
    webkit_settings_set_javascript_can_access_clipboard(settings, cfg->enable_clipboard);
    webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS);
    webkit_settings_set_enable_encrypted_media(settings, TRUE);
    webkit_settings_set_enable_media_stream(settings, TRUE);
    webkit_settings_set_enable_mediasource(settings, TRUE);

    if (g_getenv("DUCKAI_SIDEBAR_DEBUG") != NULL) {
        webkit_settings_set_enable_developer_extras(settings, TRUE);
        webkit_settings_set_enable_write_console_messages_to_stdout(settings, TRUE);
    }

    if (cfg->user_agent && *cfg->user_agent) {
        webkit_settings_set_user_agent(settings, cfg->user_agent);
    }

    GtkWidget *view = webkit_web_view_new_with_context(context);
    g_object_unref(context);

    /* Avoid initial white flash by defaulting background color to dark */
    GdkRGBA bg_color = {0.1176f, 0.1176f, 0.1804f, 1.0f};
    webkit_web_view_set_background_color(WEBKIT_WEB_VIEW(view), &bg_color);

    webkit_web_view_set_settings(WEBKIT_WEB_VIEW(view), settings);
    g_object_unref(settings);

    g_signal_connect(view, "decide-policy", G_CALLBACK(on_decide_policy), app);
    g_signal_connect(view, "create", G_CALLBACK(on_create_web_view), app);
    g_signal_connect(view, "permission-request", G_CALLBACK(on_permission_request), app);
    g_signal_connect(view, "key-press-event", G_CALLBACK(on_key_press), app);

    /* Load initial URL */
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(view), cfg->url ? cfg->url : "https://duck.ai");

    return view;
}

void webview_reload(GtkWidget *webview, gboolean bypass_cache) {
    if (!webview || !WEBKIT_IS_WEB_VIEW(webview)) return;
    if (bypass_cache) {
        webkit_web_view_reload_bypass_cache(WEBKIT_WEB_VIEW(webview));
    } else {
        webkit_web_view_reload(WEBKIT_WEB_VIEW(webview));
    }
}

void webview_zoom_in(GtkWidget *webview) {
    if (!webview || !WEBKIT_IS_WEB_VIEW(webview)) return;
    gdouble level = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(webview));
    webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(webview), level + 0.1);
}

void webview_zoom_out(GtkWidget *webview) {
    if (!webview || !WEBKIT_IS_WEB_VIEW(webview)) return;
    gdouble level = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(webview));
    if (level > 0.3) {
        webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(webview), level - 0.1);
    }
}

void webview_zoom_reset(GtkWidget *webview) {
    if (!webview || !WEBKIT_IS_WEB_VIEW(webview)) return;
    webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(webview), 1.0);
}
