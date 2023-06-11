#include "messenger.h"
#include "connection_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Структура, которая будет передаваться по каналам между клиентами
// для текстовых сообщений
struct client_message 
{
    enum msg_type type;
    char msg[MESSAGE_MAX_LENGTH];
    char sender[CLIENT_NAME_MAX_LENGTH];
};


int unconnect(client_connection *list, const char *client_name)
{
    if (list == NULL) {
        fprintf(stderr, "The list ofn connections doesn't exist\n");
        return -1;
    }

    struct client_message unconnect_data;
    unconnect_data.type = UNCONNECT_MSG;
    strncpy(unconnect_data.sender, client_name, CLIENT_NAME_MAX_LENGTH);

    client_connection *tmp = list;
    while (tmp != NULL) {
        if (tmp->client_socket != -1) {
            if (send(tmp->client_socket, &unconnect_data, sizeof(unconnect_data), 0) < 0) {
                fprintf(stderr, "Failed to send the unconnect request to clients: %s\n", strerror(errno));
                return -1;
            }
        }
    }
    
    return 0;
}
