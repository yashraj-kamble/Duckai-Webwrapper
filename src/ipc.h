#ifndef IPC_H
#define IPC_H

#include <glib.h>
#include "app.h"

gchar *ipc_get_socket_path(void);
gint ipc_client_send_command(const gchar *command, gchar **out_response);
gboolean ipc_server_start(App *app);
void ipc_server_stop(void);

#endif /* IPC_H */
