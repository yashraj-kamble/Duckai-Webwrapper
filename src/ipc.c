#define _GNU_SOURCE
#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

static int server_fd = -1;
static guint server_watch_id = 0;
static gchar *saved_socket_path = NULL;

gchar *ipc_get_socket_path(void) {
    const gchar *runtime_dir = g_get_user_runtime_dir();
    if (runtime_dir && g_file_test(runtime_dir, G_FILE_TEST_IS_DIR)) {
        return g_build_filename(runtime_dir, "duckai-sidebar.sock", NULL);
    }
    return g_strdup_printf("/tmp/duckai-sidebar-%u.sock", (guint)getuid());
}

gint ipc_client_send_command(const gchar *command, gchar **out_response) {
    gchar *sock_path = ipc_get_socket_path();
    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        g_free(sock_path);
        return -1;
    }

    /* Set 3-second timeout so client never hangs if server is unresponsive */
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        g_free(sock_path);
        return -1;
    }

    g_free(sock_path);

    /* Send command */
    gchar *cmd_with_nl = g_strdup_printf("%s\n", command);
    size_t len = strlen(cmd_with_nl);
    ssize_t written = write(fd, cmd_with_nl, len);
    g_free(cmd_with_nl);

    if (written < 0) {
        close(fd);
        return -1;
    }

    /* Read response */
    char buf[512];
    memset(buf, 0, sizeof(buf));
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (n > 0) {
        buf[n] = '\0';
        g_strchomp(buf);
        if (out_response) {
            *out_response = g_strdup(buf);
        }
        return 0;
    }

    return 0;
}

static gboolean on_quit_idle(gpointer user_data) {
    App *app = (App *)user_data;
    app_quit(app);
    return G_SOURCE_REMOVE;
}

static gboolean on_client_connected(GIOChannel *source, GIOCondition condition, gpointer user_data) {
    (void)source;
    App *app = (App *)user_data;
    if (condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
        return FALSE;
    }

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        return TRUE;
    }
    fcntl(client_fd, F_SETFD, FD_CLOEXEC);

    char buf[512];
    memset(buf, 0, sizeof(buf));
    ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
    if (n <= 0) {
        close(client_fd);
        return TRUE;
    }

    buf[n] = '\0';
    g_strchomp(buf);

    const char *reply = "OK\n";

    if (g_ascii_strcasecmp(buf, "TOGGLE") == 0) {
        app_toggle(app);
        reply = app_is_visible(app) ? "OK SHOWN\n" : "OK HIDDEN\n";
    } else if (g_ascii_strcasecmp(buf, "SHOW") == 0) {
        app_show(app);
        reply = "OK SHOWN\n";
    } else if (g_ascii_strcasecmp(buf, "HIDE") == 0) {
        app_hide(app);
        reply = "OK HIDDEN\n";
    } else if (g_ascii_strcasecmp(buf, "RELOAD") == 0) {
        app_reload(app, FALSE);
        reply = "OK RELOADED\n";
    } else if (g_ascii_strcasecmp(buf, "RELOAD_BYPASS") == 0) {
        app_reload(app, TRUE);
        reply = "OK RELOADED_BYPASS\n";
    } else if (g_ascii_strcasecmp(buf, "STATUS") == 0) {
        gchar *resp = g_strdup_printf("%s\n", app_get_status_string(app));
        ssize_t w = write(client_fd, resp, strlen(resp));
        (void)w;
        g_free(resp);
        close(client_fd);
        return TRUE;
    } else if (g_ascii_strncasecmp(buf, "SET_WIDTH ", 10) == 0) {
        int w = atoi(buf + 10);
        if (w >= 150) {
            app_set_width(app, w);
            reply = "OK WIDTH\n";
        } else {
            reply = "ERROR INVALID_WIDTH\n";
        }
    } else if (g_ascii_strncasecmp(buf, "SET_EDGE ", 9) == 0) {
        const char *edge = buf + 9;
        app_set_edge(app, edge);
        reply = "OK EDGE\n";
    } else if (g_ascii_strncasecmp(buf, "SET_MONITOR ", 12) == 0) {
        const char *mon = buf + 12;
        app_set_monitor(app, mon);
        reply = "OK MONITOR\n";
    } else if (g_ascii_strncasecmp(buf, "SET_BORDER ", 11) == 0) {
        gchar **parts = g_strsplit(buf + 11, " ", 2);
        if (parts && parts[0]) {
            int bw = atoi(parts[0]);
            const char *bcol = (parts[1] && *parts[1]) ? parts[1] : NULL;
            app_set_border(app, bw, bcol);
            reply = "OK BORDER\n";
        } else {
            reply = "ERROR INVALID_BORDER\n";
        }
        g_strfreev(parts);
    } else if (g_ascii_strcasecmp(buf, "QUIT") == 0) {
        reply = "OK QUITTING\n";
        ssize_t w = write(client_fd, reply, strlen(reply));
        (void)w;
        close(client_fd);
        g_idle_add(on_quit_idle, app);
        return TRUE;
    } else {
        reply = "ERROR UNKNOWN_COMMAND\n";
    }

    ssize_t w = write(client_fd, reply, strlen(reply));
    (void)w;
    close(client_fd);
    return TRUE;
}

gboolean ipc_server_start(App *app) {
    gchar *sock_path = ipc_get_socket_path();
    g_free(saved_socket_path);
    saved_socket_path = g_strdup(sock_path);

    /* Unlink any dead socket file */
    unlink(sock_path);

    server_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (server_fd < 0) {
        g_warning("Failed to create IPC socket: %s", strerror(errno));
        g_free(sock_path);
        return FALSE;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        g_warning("Failed to bind IPC socket to %s: %s", sock_path, strerror(errno));
        close(server_fd);
        server_fd = -1;
        g_free(sock_path);
        return FALSE;
    }

    if (listen(server_fd, 8) < 0) {
        g_warning("Failed to listen on IPC socket: %s", strerror(errno));
        close(server_fd);
        server_fd = -1;
        unlink(sock_path);
        g_free(sock_path);
        return FALSE;
    }

    GIOChannel *channel = g_io_channel_unix_new(server_fd);
    server_watch_id = g_io_add_watch(channel, G_IO_IN, on_client_connected, app);
    g_io_channel_unref(channel);

    g_free(sock_path);
    return TRUE;
}

void ipc_server_stop(void) {
    if (server_watch_id > 0) {
        g_source_remove(server_watch_id);
        server_watch_id = 0;
    }
    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }
    if (saved_socket_path) {
        unlink(saved_socket_path);
        g_free(saved_socket_path);
        saved_socket_path = NULL;
    }
}
