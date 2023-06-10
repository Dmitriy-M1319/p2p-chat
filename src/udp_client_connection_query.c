#include "udp_client_connection_query.h"
#include "connection_list.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#define DEBUG_LOCAL


int create_udb_broadcast_socket()
{
    int sock;
    if((sock = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Failed to create socket for invitation in network: %s\n", strerror(errno));
        return -1;
    }

    int broadcast = 1;
    int timeout = 6000;

    if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1) {
        fprintf(stderr, "Failed to set socket for broadcast for invitation in network: %s\n", strerror(errno));
        return -1;
    }

    if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1) {
        fprintf(stderr, "Failed to set socket for timeout for invitation in network: %s\n", strerror(errno));
        return -1;
    }

    return sock;
}


int create_simple_udp_socket()
{
    int sock;
    if((sock = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Failed to create socket for response to client: %s\n", strerror(errno));
        return -1;
    }

    return sock;
}


int send_connection_query(int udp_socket, const char *nickname)
{
    int status;
    struct addrinfo hints, *addr_res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE;

#ifdef DEBUG_LOCAL
    if((status = getaddrinfo(NULL, UDP_BROADCAST_PORT, &hints, &addr_res)) != 0) {
#else
    if((status = getaddrinfo(BROADCAST_ADDR, UDP_BROADCAST_PORT, &hints, &addr_res)) != 0) {
#endif
        fprintf(stderr, "Error with getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }
    for (struct addrinfo *p = addr_res; p != NULL; p = p->ai_next) {
        if (p->ai_family == AF_INET) {
            // Создаем структуру с данными для отправки
            struct query_datagramm dgram;
            dgram.port = 0;
            strncpy(dgram.msg, CONNECTION_UDP_REQUEST, sizeof(CONNECTION_UDP_REQUEST));

            // и теперь отсылаем данные на запрос
            if (sendto(udp_socket, &dgram, sizeof(dgram), 0, addr_res->ai_addr, addr_res->ai_addrlen) == -1) {
                fprintf(stderr, "Failed to send query for invitation in network: %s\n", strerror(errno));
                return -1;
            }
            break;
        }
    }
    freeaddrinfo(addr_res);
    return 0;
}


int create_tcp_client_socket()
{
    int sock, status;
    struct addrinfo hints, *addr_res;

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Failed to create TCP socket for new client: %s\n", strerror(errno));
        return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE;

#ifdef DEBUG_LOCAL
    if((status = getaddrinfo(NULL, UDP_NEW_CLIENT_PORT, &hints, &addr_res)) != 0) {
#else
    if((status = getaddrinfo(BROADCAST_ADDR, UDP_NEW_CLIENT_PORT &hints, &addr_res)) != 0) {
#endif
        fprintf(stderr, "Error with getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    for (struct addrinfo *p = addr_res; p != NULL; p = p->ai_next) {
        if (p->ai_family == AF_INET) {
            if((status = bind(sock, addr_res->ai_addr, addr_res->ai_addrlen)) == -1) {
                fprintf(stderr, "Failed to bind socket for new client: %s\n", strerror(errno));
                return -1;
            }   
            break;
        }
    }

    freeaddrinfo(addr_res);
    return sock;
}


int send_connection_response(int udp_socket, struct sockaddr_in *client_info, int client_port)
{
    int status;
    struct addrinfo hints, *addr_res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE;

#ifdef DEBUG_LOCAL
    if((status = getaddrinfo("localhost", NULL, &hints, &addr_res)) != 0) {
#else
    if((status = getaddrinfo(BROADCAST_ADDR, NULL, &hints, &addr_res)) != 0) {
#endif
        fprintf(stderr, "Error with getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    for (struct addrinfo *p = addr_res; p != NULL; p = p->ai_next) {
        if (p->ai_family == AF_INET) {
            // Создаем структуру с данными для отправки
            struct query_datagramm dgram;
            dgram.port = client_port;
            strncpy(dgram.msg, CONNECTION_UDP_RESPONSE, sizeof(CONNECTION_UDP_RESPONSE));

            if (sendto(udp_socket, &dgram, sizeof(dgram), 0, addr_res->ai_addr, addr_res->ai_addrlen) == -1) {
                fprintf(stderr, "Failed to send response to new client: %s\n", strerror(errno));
                return -1;
            }

            break;
        }
    }
    freeaddrinfo(addr_res);
    return 0;
}

int create_client_connection(struct query_datagramm *data, client_connection *connections)
{
    int status, new_tcp_client_socket;
    char port[5];
    struct addrinfo hints, *client_addr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;

    sprintf(port, "%d", data->port);

    if((status = getaddrinfo(data->address, port, &hints, &client_addr)) != 0) {
        fprintf(stderr, "Error with getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    new_tcp_client_socket = socket(client_addr->ai_family, 
            client_addr->ai_socktype, 
            client_addr->ai_protocol);

    if(connect(new_tcp_client_socket, client_addr->ai_addr, client_addr->ai_addrlen) != 0) {
        fprintf(stderr, "Failed to connect to new client: %s\n", strerror(errno));
        return -1;
    }

    add_new_connection(connections, data->nickname, new_tcp_client_socket, (struct sockaddr_in *)client_addr->ai_addr);

    return new_tcp_client_socket;
}
