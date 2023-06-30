#include "connection_list.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


struct client_connection_node *create_connections()
{
    struct client_connection_node *head = (struct client_connection_node *)malloc(sizeof(struct client_connection_node));
    strncpy(head->client_name, " ", CLIENT_NAME_MAX_LENGTH);
    head->client_socket = -1;
    head->next = NULL;
    return head;
}

void print_connections_ptr(client_connection *connections_ptr)
{
    struct client_connection_node *connection_ptr = connections_ptr;
    while (connection_ptr != NULL) {
        if (connection_ptr->client_socket != -1) {
            printf("Connection: %s\n", connection_ptr->client_name);
            char local_decimal_addr[INET_ADDRSTRLEN];
            inet_ntop(connection_ptr->client_address_info.sin_family, 
                    &connection_ptr->client_address_info.sin_addr, local_decimal_addr, INET_ADDRSTRLEN);
            printf("IPv4: %s:%d\n", local_decimal_addr, ntohs(connection_ptr->client_address_info.sin_port));
            puts("");

        }
        connection_ptr = connection_ptr->next;
    }
}


client_connection *add_new_connection(struct client_connection_node *connections, 
        const char *client, 
        const int socket, 
        const struct sockaddr_in *addr_info)
{
    if (connections == NULL) {
        return NULL;
    }
    struct client_connection_node *connection_ptr = connections;
    while (connection_ptr->next != NULL) {
        connection_ptr = connection_ptr->next;
    }

    if((connection_ptr->next = (struct client_connection_node *)malloc(sizeof(struct client_connection_node))) == NULL ) {
        return NULL;
    }

    strncpy(connection_ptr->next->client_name, client, CLIENT_NAME_MAX_LENGTH);
    connection_ptr->next->client_socket = socket;
    memcpy(&connection_ptr->next->client_address_info, addr_info, sizeof(struct sockaddr_in));
    connection_ptr->next->next = NULL;

    return connection_ptr->next;
}

client_connection *add_new_secure_connection(struct client_connection_node *connections, 
        const char *client, 
        const int socket, 
        const struct sockaddr_in *addr_info, 
        SSL *ssl_object, 
        SSL_CTX *context)
{
    if (connections == NULL) {
        return NULL;
    }
    struct client_connection_node *connection_ptr = connections;
    while (connection_ptr->next != NULL) {
        connection_ptr = connection_ptr->next;
    }

    if((connection_ptr->next = (struct client_connection_node *)malloc(sizeof(struct client_connection_node))) == NULL ) {
        return NULL;
    }

    strncpy(connection_ptr->next->client_name, client, CLIENT_NAME_MAX_LENGTH);
    connection_ptr->next->client_socket = socket;
    memcpy(&connection_ptr->next->client_address_info, addr_info, sizeof(struct sockaddr_in));
    connection_ptr->next->ssl_object = ssl_object;
    connection_ptr->next->context = context;
    connection_ptr->next->next = NULL;
    return connection_ptr->next;
}

struct client_connection_node *get_connection(struct client_connection_node *connections, const char *client)
{
    if (connections == NULL) {
        return NULL;
    }
    struct client_connection_node *connection_ptr = connections;
    while (connection_ptr != NULL) {
        if (!strncmp(connection_ptr->client_name, client, CLIENT_NAME_MAX_LENGTH)) {
            break;
        }
        connection_ptr = connection_ptr->next;
    }

    return connection_ptr;
}


int remove_connection(struct client_connection_node *connections, const char *client)
{
    if (connections == NULL) {
        return -1;
    }
    struct client_connection_node *connection_ptr = connections;
    struct client_connection_node *destroyed = NULL;
    while (connection_ptr->next != NULL) {
        if (!strncmp(connection_ptr->next->client_name, client, CLIENT_NAME_MAX_LENGTH)) {
            break;
        }
        connection_ptr = connection_ptr->next;
    }
    destroyed = connection_ptr->next;
    connection_ptr->next = destroyed->next;

    close(destroyed->client_socket);
    if (destroyed->ssl_object) {
        SSL_free(destroyed->ssl_object);
        SSL_CTX_free(destroyed->context);
    }
    free(destroyed);
    return 0;
}


int free_connections(struct client_connection_node **connections_ptr)
{
    if ((*connections_ptr) == NULL) {
        return -1;
    }
    struct client_connection_node *destroyed = NULL;
    while ((*connections_ptr)->next != NULL) {
        destroyed = *connections_ptr;
        *connections_ptr = (*connections_ptr)->next;

        if (destroyed->client_socket != -1) {
            close(destroyed->client_socket);
        }
        if (destroyed->ssl_object) {
            SSL_free(destroyed->ssl_object);
            SSL_CTX_free(destroyed->context);
        }
        free(destroyed);
    }

    if ((*connections_ptr)->client_socket != -1) {
        close((*connections_ptr)->client_socket);
    }
    if ((*connections_ptr)->ssl_object) {
        SSL_free((*connections_ptr)->ssl_object);
        SSL_CTX_free((*connections_ptr)->context);
    }
    free(*connections_ptr);
    *connections_ptr = NULL;
    return 0; 
}
