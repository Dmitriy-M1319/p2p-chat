#include "connection_list.h"
#include <string.h>
#include <stdlib.h>


int initialize_client_list(struct client_connection_node *head)
{
    if((head = (struct client_connection_node *)malloc(sizeof(struct client_connection_node))) == NULL) {
        return -1;
    }
    return 0;
}


int add_new_connection(struct client_connection_node *list, const char *name, int socket, const struct sockaddr_in *addr)
{
    if (list == NULL) {
        return -1;
    }
    struct client_connection_node *tmp = list;
    while (tmp->next != NULL) {
        tmp = tmp->next;
    }

    if((tmp->next = (struct client_connection_node *)malloc(sizeof(struct client_connection_node))) == NULL ) {
        return -1;
    }

    strncpy(tmp->next->client_name, name, CLIENT_NAME_MAX_LENGTH);
    tmp->next->client_socket = socket;
    memcpy(&tmp->next->client_address_info, addr, sizeof(struct sockaddr_in));

    return 0;
}
