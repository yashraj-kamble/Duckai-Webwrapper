#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "config.h"
#include "app.h"
#include "ipc.h"

#define VERSION "2.0.0"

static App *global_app = NULL;

static void handle_signal(int sig) {
    (void)sig;
    if (global_app) {
        app_quit(global_app);
    } else {
        ipc_server_stop();
        exit(0);
    }
}

static void print_usage(const char *prog) {
    printf("duckai-sidebar %s - Native Wayland sidebar panel for Duck.ai\n\n", VERSION);
    printf("Usage:\n");
    printf("  %s [OPTIONS]\n\n", prog);
    printf("Actions (IPC):\n");
    printf("  -t, --toggle           Toggle sidebar visibility (starts daemon if not running)\n");
    printf("  -s, --show             Show the sidebar (starts daemon if not running)\n");
    printf("      --hide             Hide the sidebar\n");
    printf("  -r, --reload           Reload Duck.ai page\n");
    printf("      --reload-bypass    Reload Duck.ai bypassing cache\n");
    printf("  -q, --quit             Terminate the running daemon\n");
    printf("      --status           Show running status and visibility\n\n");
    printf("Configuration & Options:\n");
    printf("  -w, --width <px>       Set sidebar width in pixels (default: 420)\n");
    printf("  -e, --edge <left|right> Anchor to left or right edge (default: right)\n");
    printf("  -m, --monitor <name>   Monitor to place sidebar on ('active' or name like 'eDP-1')\n");
    printf("      --border-width <px> Set border width in pixels (default: 2)\n");
    printf("      --border-color <col> Set border color (e.g. '#33ccff', default: theme accent)\n");
    printf("  -H, --hidden           Start daemon in hidden state (useful for exec-once)\n");
    printf("      --header           Show minimal navigation header bar\n");
    printf("      --no-header        Hide navigation header bar (pure webview, default)\n");
    printf("  -u, --url <url>        Custom URL (default: https://duck.ai)\n");
    printf("  -c, --config <file>    Path to custom configuration INI file\n");
    printf("      --dump-config      Print parsed configuration and exit\n");
    printf("  -v, --version          Print version information\n");
    printf("  -h, --help             Show this help message\n\n");
    printf("Hyprland shortcut example:\n");
    printf("  bind = $mainMod, D, exec, duckai-sidebar --toggle\n");
}

int main(int argc, char **argv) {
    gboolean opt_toggle = FALSE;
    gboolean opt_show = FALSE;
    gboolean opt_hide = FALSE;
    gboolean opt_reload = FALSE;
    gboolean opt_reload_bypass = FALSE;
    gboolean opt_quit = FALSE;
    gboolean opt_status = FALSE;
    gboolean opt_hidden = FALSE;
    gboolean opt_header = FALSE;
    gboolean opt_no_header = FALSE;
    gboolean opt_dump_config = FALSE;
    gboolean opt_version = FALSE;
    gboolean opt_help = FALSE;

    gint opt_width = 0;
    gchar *opt_edge = NULL;
    gchar *opt_monitor = NULL;
    gchar *opt_url = NULL;
    gchar *opt_config = NULL;
    gint opt_border_width = 0;
    gchar *opt_border_color = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--toggle") == 0) {
            opt_toggle = TRUE;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--show") == 0) {
            opt_show = TRUE;
        } else if (strcmp(argv[i], "--hide") == 0) {
            opt_hide = TRUE;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reload") == 0) {
            opt_reload = TRUE;
        } else if (strcmp(argv[i], "--reload-bypass") == 0) {
            opt_reload_bypass = TRUE;
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quit") == 0) {
            opt_quit = TRUE;
        } else if (strcmp(argv[i], "--status") == 0) {
            opt_status = TRUE;
        } else if (strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "--hidden") == 0) {
            opt_hidden = TRUE;
        } else if (strcmp(argv[i], "--header") == 0) {
            opt_header = TRUE;
        } else if (strcmp(argv[i], "--no-header") == 0) {
            opt_no_header = TRUE;
        } else if (strcmp(argv[i], "--dump-config") == 0) {
            opt_dump_config = TRUE;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            opt_version = TRUE;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            opt_help = TRUE;
        } else if ((strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0) && i + 1 < argc) {
            opt_width = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--edge") == 0) && i + 1 < argc) {
            g_free(opt_edge);
            opt_edge = g_strdup(argv[++i]);
        } else if ((strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--monitor") == 0) && i + 1 < argc) {
            g_free(opt_monitor);
            opt_monitor = g_strdup(argv[++i]);
        } else if (strcmp(argv[i], "--border-width") == 0 && i + 1 < argc) {
            opt_border_width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--border-color") == 0 && i + 1 < argc) {
            g_free(opt_border_color);
            opt_border_color = g_strdup(argv[++i]);
        } else if ((strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--url") == 0) && i + 1 < argc) {
            g_free(opt_url);
            opt_url = g_strdup(argv[++i]);
        } else if ((strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) && i + 1 < argc) {
            g_free(opt_config);
            opt_config = g_strdup(argv[++i]);
        } else {
            fprintf(stderr, "Unknown option: %s\nRun '%s --help' for available options.\n", argv[i], argv[0]);
            g_free(opt_edge);
            g_free(opt_monitor);
            g_free(opt_border_color);
            g_free(opt_url);
            g_free(opt_config);
            return 1;
        }
    }

    if (opt_help) {
        print_usage(argv[0]);
        g_free(opt_edge);
        g_free(opt_monitor);
        g_free(opt_border_color);
        g_free(opt_url);
        g_free(opt_config);
        return 0;
    }

    if (opt_version) {
        printf("duckai-sidebar %s\n", VERSION);
        g_free(opt_edge);
        g_free(opt_monitor);
        g_free(opt_border_color);
        g_free(opt_url);
        g_free(opt_config);
        return 0;
    }

    /* ── Dump Configuration ──────────────────────────────────────────────── */
    if (opt_dump_config) {
        SidebarConfig *cfg = config_new();
        if (opt_config) {
            config_load_file(cfg, opt_config);
        } else {
            config_load_default(cfg);
        }
        if (opt_width > 0) cfg->width = opt_width;
        if (opt_edge) { g_free(cfg->edge); cfg->edge = g_strdup(opt_edge); }
        if (opt_monitor) { g_free(cfg->monitor); cfg->monitor = g_strdup(opt_monitor); }
        if (opt_url) { g_free(cfg->url); cfg->url = g_strdup(opt_url); }
        if (opt_header) cfg->show_header = TRUE;
        if (opt_no_header) cfg->show_header = FALSE;
        if (opt_border_width > 0) cfg->border_width = opt_border_width;
        if (opt_border_color) { g_free(cfg->border_color); cfg->border_color = g_strdup(opt_border_color); }
        config_print(cfg);
        config_free(cfg);
        g_free(opt_edge);
        g_free(opt_monitor);
        g_free(opt_border_color);
        g_free(opt_url);
        g_free(opt_config);
        return 0;
    }

    /* ── IPC Client Dispatch ──────────────────────────────────────────────── */
    gchar *resp = NULL;
    gboolean is_running = (ipc_client_send_command("STATUS", &resp) == 0);
    g_free(resp);
    resp = NULL;

    if (is_running) {
        /* Apply dynamic property updates to running instance */
        gboolean updated_prop = FALSE;

        if (opt_width > 0) {
            gchar *cmd = g_strdup_printf("SET_WIDTH %d", opt_width);
            if (ipc_client_send_command(cmd, &resp) == 0) {
                printf("Sidebar width updated to %d px.\n", opt_width);
                g_free(resp);
                resp = NULL;
                updated_prop = TRUE;
            }
            g_free(cmd);
        }

        if (opt_edge) {
            gchar *cmd = g_strdup_printf("SET_EDGE %s", opt_edge);
            if (ipc_client_send_command(cmd, &resp) == 0) {
                printf("Sidebar edge updated to %s.\n", opt_edge);
                g_free(resp);
                resp = NULL;
                updated_prop = TRUE;
            }
            g_free(cmd);
        }

        if (opt_monitor) {
            gchar *cmd = g_strdup_printf("SET_MONITOR %s", opt_monitor);
            if (ipc_client_send_command(cmd, &resp) == 0) {
                printf("Sidebar monitor updated to %s.\n", opt_monitor);
                g_free(resp);
                resp = NULL;
                updated_prop = TRUE;
            }
            g_free(cmd);
        }

        if (opt_border_width > 0 || opt_border_color) {
            gchar *cmd = g_strdup_printf("SET_BORDER %d %s",
                                         opt_border_width >= 0 ? opt_border_width : 2,
                                         opt_border_color ? opt_border_color : "");
            if (ipc_client_send_command(cmd, &resp) == 0) {
                printf("Sidebar border updated.\n");
                g_free(resp);
                resp = NULL;
                updated_prop = TRUE;
            }
            g_free(cmd);
        }

        if (opt_reload) {
            if (ipc_client_send_command("RELOAD", &resp) == 0) {
                printf("Duck.ai sidebar reloaded.\n");
                g_free(resp);
            }
            goto cleanup_and_exit;
        }

        if (opt_reload_bypass) {
            if (ipc_client_send_command("RELOAD_BYPASS", &resp) == 0) {
                printf("Duck.ai sidebar reloaded (cache bypassed).\n");
                g_free(resp);
            }
            goto cleanup_and_exit;
        }

        if (opt_quit) {
            if (ipc_client_send_command("QUIT", &resp) == 0) {
                printf("Duck.ai sidebar daemon terminated.\n");
                g_free(resp);
            }
            goto cleanup_and_exit;
        }

        if (opt_status) {
            if (ipc_client_send_command("STATUS", &resp) == 0) {
                printf("%s\n", resp ? resp : "OK");
                g_free(resp);
            }
            goto cleanup_and_exit;
        }

        if (opt_hide) {
            if (ipc_client_send_command("HIDE", &resp) == 0) {
                printf("Duck.ai sidebar hidden.\n");
                g_free(resp);
            }
            goto cleanup_and_exit;
        }

        if (opt_toggle) {
            if (ipc_client_send_command("TOGGLE", &resp) == 0) {
                if (resp && strstr(resp, "SHOWN")) {
                    printf("Duck.ai sidebar shown.\n");
                } else {
                    printf("Duck.ai sidebar hidden.\n");
                }
                g_free(resp);
            }
            goto cleanup_and_exit;
        }

        if (opt_show) {
            if (ipc_client_send_command("SHOW", &resp) == 0) {
                printf("Duck.ai sidebar shown.\n");
                g_free(resp);
            }
            goto cleanup_and_exit;
        }

        if (updated_prop) {
            goto cleanup_and_exit;
        }

        /* Default action when already running: bring to front */
        if (!opt_hidden) {
            if (ipc_client_send_command("SHOW", &resp) == 0) {
                printf("Duck.ai sidebar already running; brought to front.\n");
                g_free(resp);
            }
            goto cleanup_and_exit;
        }
    } else {
        /* Not running: actions that require a running daemon report accordingly */
        if (opt_quit) {
            printf("duckai-sidebar is not running.\n");
            goto cleanup_and_exit;
        }
        if (opt_status) {
            printf("duckai-sidebar is not running.\n");
            g_free(opt_edge);
            g_free(opt_monitor);
            g_free(opt_border_color);
            g_free(opt_url);
            g_free(opt_config);
            return 1;
        }
        if (opt_hide) {
            printf("duckai-sidebar is not running.\n");
            goto cleanup_and_exit;
        }
        if (opt_reload || opt_reload_bypass) {
            fprintf(stderr, "duckai-sidebar is not running.\n");
            g_free(opt_edge);
            g_free(opt_monitor);
            g_free(opt_border_color);
            g_free(opt_url);
            g_free(opt_config);
            return 1;
        }
    }

    /* ── Server Startup ──────────────────────────────────────────────────── */
    gtk_init(&argc, &argv);

    /* Load Configuration */
    SidebarConfig *cfg = config_new();
    if (opt_config) {
        config_load_file(cfg, opt_config);
    } else {
        config_load_default(cfg);
    }

    /* Apply command line overrides */
    if (opt_width > 0) cfg->width = opt_width;
    if (opt_edge) {
        g_free(cfg->edge);
        cfg->edge = opt_edge;
        opt_edge = NULL;
    }
    if (opt_monitor) {
        g_free(cfg->monitor);
        cfg->monitor = opt_monitor;
        opt_monitor = NULL;
    }
    if (opt_url) {
        g_free(cfg->url);
        cfg->url = opt_url;
        opt_url = NULL;
    }
    if (opt_header) cfg->show_header = TRUE;
    if (opt_no_header) cfg->show_header = FALSE;
    if (opt_border_width > 0) cfg->border_width = opt_border_width;
    if (opt_border_color) {
        g_free(cfg->border_color);
        cfg->border_color = opt_border_color;
        opt_border_color = NULL;
    }

    /* Setup signal handlers */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Create Application and Layer Shell Window */
    App *app = app_create(cfg);
    if (!app) {
        fprintf(stderr, "Failed to initialize duckai-sidebar!\n");
        config_free(cfg);
        g_free(opt_edge);
        g_free(opt_monitor);
        g_free(opt_border_color);
        g_free(opt_url);
        g_free(opt_config);
        return 1;
    }
    global_app = app;

    /* Start IPC server */
    if (!ipc_server_start(app)) {
        fprintf(stderr, "Failed to start IPC server!\n");
        app_destroy(app);
        config_free(cfg);
        g_free(opt_edge);
        g_free(opt_monitor);
        g_free(opt_border_color);
        g_free(opt_url);
        g_free(opt_config);
        return 1;
    }

    /* Determine initial visibility */
    if (opt_hidden) {
        app_hide(app);
        printf("duckai-sidebar %s started in background (hidden).\n", VERSION);
    } else {
        app_show(app);
        printf("duckai-sidebar %s started and shown.\n", VERSION);
    }

    /* Run GTK main loop */
    gtk_main();

    /* Clean exit */
    ipc_server_stop();
    app_destroy(app);
    config_free(cfg);

cleanup_and_exit:
    g_free(opt_edge);
    g_free(opt_monitor);
    g_free(opt_border_color);
    g_free(opt_url);
    g_free(opt_config);
    return 0;
}
