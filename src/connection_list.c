#include "connection_list.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


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


struct client_connection_node *get_client_info(struct client_connection_node *list, const char *name)
{
    if (list == NULL) {
        return NULL;
    }
    struct client_connection_node *tmp = list;
    while (tmp->next != NULL || strncmp(tmp->client_name, name, CLIENT_NAME_MAX_LENGTH)) {
        tmp = tmp->next;
    }

    return tmp;
}


int remove_connection(struct client_connection_node *list, const char *name)
{
    if (list == NULL) {
        return -1;
    }
    struct client_connection_node *tmp = list;
    struct client_connection_node *destroyed = NULL;
    while (tmp->next != NULL || strncmp(tmp->next->client_name, name, CLIENT_NAME_MAX_LENGTH)) {
        tmp = tmp->next;
    }
    destroyed = tmp->next;
    tmp->next = destroyed->next;

    // Закрываем сокет
    close(destroyed->client_socket);
    free(destroyed);
    return 0;
}


int free_connection_list(struct client_connection_node *list)
{
    if (list == NULL) {
        return -1;
    }
    struct client_connection_node *destroyed = NULL;
    while (list->next != NULL) {
        destroyed = list;
        list = list->next;

        // Закрываем сокет
        close(destroyed->client_socket);
        free(destroyed);
    }

    return 0; 
}
