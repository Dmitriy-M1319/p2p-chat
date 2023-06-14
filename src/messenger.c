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
    FILE *fd;
    long bytes_send = 0;
    long filesize;
    char buf[MESSAGE_MAX_LENGTH];
    struct client_message file_part;
    client_connection *client = NULL;

    if ((client = get_client_info(list, receiver)) == NULL) {
        fprintf(stderr, "No such client for sending\n");
        return -1;
    }

    file_part.type = FILE_MSG;
    strncpy(file_part.filename, filename, MESSAGE_MAX_LENGTH);

    fd = fopen(filename, "rb");
    fseek(fd, 0L, SEEK_END);
    filesize = ftell(fd);
    rewind(fd);
    file_part.size = filesize;

    if (send(client->client_socket, &file_part, sizeof(file_part), 0) < 0){
        perror("send file size");
        fclose(fd);
        return 1;
    }

    while (bytes_send < filesize) {
        int bytes_to_read = (filesize - bytes_send < MESSAGE_MAX_LENGTH) ? filesize - bytes_send : MESSAGE_MAX_LENGTH;
        bytes_send += fread(buf, 1, bytes_to_read, fd);
        
        file_part.size = bytes_to_read;
        memcpy(file_part.msg, buf, bytes_to_read);
        if (send(client->client_socket, &file_part, sizeof(file_part), 0) < 0) {
            fprintf(stderr, "Failed to send a file for client %s: %s\n", 
                    client->client_name, 
                    strerror(errno));
            fclose(fd);
            return -1;
        }
    }

    fclose(fd);
    return 0;
}
