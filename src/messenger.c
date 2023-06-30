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
#include <openssl/err.h>


int unconnect(client_connection *connections)
{
    if (connections == NULL) {
        fprintf(stderr, "The list ofn connections doesn't exist\n");
        return -1;
    }

    struct client_message msg_datagramm;
    msg_datagramm.type = UNCONNECT_QUERY_MSG;

    client_connection *connection_ptr = connections;
    while (connection_ptr != NULL) {
        if (connection_ptr->client_socket != -1) {
            if (SSL_write(connection_ptr->ssl_object, &msg_datagramm, sizeof(msg_datagramm)) <= 0) {
                fprintf(stderr, "Failed to send the unconnect request to clients\n");
                ERR_print_errors_fp(stderr);
                return -1;
            }
        }
        connection_ptr = connection_ptr->next;
    }
    
    return 0;
}


int send_msg(client_connection *connections, const char *optional_client, const char *msg)
{
    struct client_message msg_datagramm;
    strncpy(msg_datagramm.msg, msg, strlen(msg));
    if (optional_client == NULL) {
        msg_datagramm.type = ALL_CLIENTS_MSG;
        client_connection *connection_ptr = connections;
        while (connection_ptr != NULL) {
            if (connection_ptr->client_socket != -1) {
                if (SSL_write(connection_ptr->ssl_object, &msg_datagramm, sizeof(msg_datagramm)) <= 0) {
                    fprintf(stderr, "Failed to send a message for clients\n");
                    ERR_print_errors_fp(stderr);
                    return -1;
                }
            }
            connection_ptr = connection_ptr->next;
        }
    } else {
        msg_datagramm.type = PRIVATE_CLIENT_MSG;
        client_connection *connection_for_client = NULL;
        if ((connection_for_client = get_connection(connections, optional_client)) == NULL) {
            fprintf(stderr, "No such client for sending");
            return -1;
        }
        if (SSL_write(connection_for_client->ssl_object, &msg_datagramm, sizeof(msg_datagramm)) <= 0) {
            fprintf(stderr, "Failed to send a message for client %s\n", connection_for_client->client_name);
            ERR_print_errors_fp(stderr);
            return -1;
        }
    }
    return 0;
}


int send_file_to_client(client_connection *connections, const char *filename, const char *client)
{
    FILE *file_descriptor;
    long bytes_send = 0;
    long filesize;
    char file_data_buffer[MESSAGE_MAX_LENGTH] = {0};
    char filename_buffer[MESSAGE_MAX_LENGTH];
    struct client_message file_part_datagramm;
    client_connection *connection_for_client = NULL;

    if ((connection_for_client = get_connection(connections, client)) == NULL) {
        fprintf(stderr, "No such client for sending\n");
        return -1;
    }

    file_part_datagramm.type = FILE_SEND_MSG;
    strncpy(file_part_datagramm.filename, filename, MESSAGE_MAX_LENGTH);

    strncpy(filename_buffer, filename, strlen(filename));

    file_descriptor = fopen(filename_buffer, "rb");
    fseek(file_descriptor, 0L, SEEK_END);
    filesize = ftell(file_descriptor);
    rewind(file_descriptor);
    file_part_datagramm.filesize = filesize;

    if (SSL_write(connection_for_client->ssl_object, &file_part_datagramm, sizeof(file_part_datagramm)) <= 0){
        fprintf(stderr, "Failed to send a file size for client %s\n", connection_for_client->client_name);
        ERR_print_errors_fp(stderr);
        fclose(file_descriptor);
        return 1;
    }

    while (bytes_send < filesize) {
        int bytes_to_read = (filesize - bytes_send < MESSAGE_MAX_LENGTH) ? filesize - bytes_send : MESSAGE_MAX_LENGTH;
        int read = fread(file_data_buffer, 1, bytes_to_read, file_descriptor);
        bytes_send += read;
        
        if (SSL_write(connection_for_client->ssl_object, &file_data_buffer, read) <= 0) {
            fprintf(stderr, "Failed to send a file for client %s\n", connection_for_client->client_name);
            ERR_print_errors_fp(stderr);
            fclose(file_descriptor);
            return -1;
        }
    }

    fclose(file_descriptor);
    return 0;
}
