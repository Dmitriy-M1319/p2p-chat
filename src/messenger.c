#include "messenger.h"
#include "connection_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


int unconnect(client_connection *list)
{
    if (list == NULL) {
        fprintf(stderr, "The list ofn connections doesn't exist\n");
        return -1;
    }

    struct client_message unconnect_data;
    unconnect_data.type = UNCONNECT_MSG;

    client_connection *tmp = list;
    while (tmp != NULL) {
        if (tmp->client_socket != -1) {
            if (send(tmp->client_socket, &unconnect_data, sizeof(unconnect_data), 0) < 0) {
                fprintf(stderr, "Failed to send the unconnect request to clients: %s\n", strerror(errno));
                return -1;
            }
        }
        tmp = tmp->next;
    }
    
    return 0;
}


int send_msg(client_connection *list, const char *receiver, const char *msg)
{
    struct client_message msg_packet;
    strncpy(msg_packet.msg, msg, strlen(msg));

    if (receiver == NULL) {
        msg_packet.type = ALL_CLIENTS_MSG;
        // Отправляем сообщение всем клиентам
        client_connection *tmp = list;
        while (tmp != NULL) {
            if (tmp->client_socket != -1) {
                if (send(tmp->client_socket, &msg_packet, sizeof(msg_packet), 0) < 0) {
                    fprintf(stderr, "Failed to send a message for clients: %s\n", strerror(errno));
                    return -1;
                }
            }
            tmp = tmp->next;
        }
    } else {
        msg_packet.type = PRIVATE_MSG;
        client_connection *client = NULL;
        if ((client = get_client_info(list, receiver)) == NULL) {
            fprintf(stderr, "No such client for sending");
            return -1;
        }
        if (send(client->client_socket, &msg_packet, sizeof(msg_packet), 0) < 0) {
            fprintf(stderr, "Failed to send a message for client %s: %s\n", 
                    client->client_name, 
                    strerror(errno));
            return -1;
        }
    }
    return 0;
}


int send_file(client_connection *list, const char *filename, const char *receiver)
{
    int file_fd = open(filename, O_RDONLY);
    int bytes_read;
    char buf[MESSAGE_MAX_LENGTH];
    struct client_message file_part;
    client_connection *client = NULL;
    struct stat file_stat; 

    if ((client = get_client_info(list, receiver)) == NULL) {
        fprintf(stderr, "No such client for sending\n");
        close(file_fd);
        return -1;
    }

    file_part.type = FILE_MSG;
    strncpy(file_part.filename, filename, MESSAGE_MAX_LENGTH);

    if (stat(filename, &file_stat) < 0) {
        fprintf(stderr, "Failed to get information by file\n");
        close(file_fd);
        return -1;
    }

    file_part.size = file_stat.st_size;
    
    while ((bytes_read = read(file_fd, buf, MESSAGE_MAX_LENGTH)) != 0) {
        strncpy(file_part.msg, buf, MESSAGE_MAX_LENGTH);
        if (send(client->client_socket, &file_part, sizeof(file_part), 0) < 0) {
            fprintf(stderr, "Failed to send a file for client %s: %s\n", 
                    client->client_name, 
                    strerror(errno));
            close(file_fd);
            return -1;
        }
    }

    close(file_fd);
    return 0;
}
