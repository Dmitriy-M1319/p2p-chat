#include "udp_client_connection_query.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>


int create_udb_broadcast_socket()
{
    int sock;
    if((sock = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Failed to create socket for invitation in network: %s\n", strerror(errno));
        return -1;
    }

    int broadcast = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1) {
        fprintf(stderr, "Failed to set socket for broadcast for invitation in network: %s\n", strerror(errno));
        return -1;
    }

    // устанавливаем время ожидания ответа в 5 секунд
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt");
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
    struct sockaddr_in local_client;
    socklen_t length;

    if (get_local_addr(&local_client, &length) == -1) {
        return -1;
    }

    local_client.sin_port = htons((uint16_t)atoi(UDP_BROADCAST_PORT));
    struct query_datagramm dgram;
    dgram.port = 0;
    strncpy(dgram.msg, CONNECTION_UDP_REQUEST, sizeof(CONNECTION_UDP_REQUEST));
    strcpy(dgram.nickname, nickname);

    if (sendto(udp_socket, &dgram, sizeof(dgram), 0, (struct sockaddr *)&local_client, length) == -1) {
        fprintf(stderr, "Failed to send query for invitation in network: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}


int get_local_addr(struct sockaddr_in *addr_out, socklen_t *length)
{
    char hostname[1024];
    int status;
    struct addrinfo hints, *local_addr;
    if(gethostname(hostname, 1024) == -1){
        fprintf(stderr, "Failed to get hostname for client machine: %s\n", strerror(errno));
        return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    if ((status = getaddrinfo(hostname, NULL, &hints, &local_addr)) == -1) {
        fprintf(stderr, "Error with getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    char ip_str[INET_ADDRSTRLEN];
    for (struct addrinfo *p = local_addr; p != NULL; p = p->ai_next) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        struct in_addr *addr = &(ipv4->sin_addr);
        inet_ntop(p->ai_family, addr, ip_str, sizeof(ip_str));

        if (strstr(ip_str, "192")) {
            memcpy(addr_out, ipv4, sizeof(struct sockaddr_in));
            memcpy(length, &p->ai_addrlen, sizeof(socklen_t));
            freeaddrinfo(local_addr);
            return 0;
        }
    }

    freeaddrinfo(local_addr);
    return -1;
}


int create_tcp_client_socket()
{
    int sock, status;
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Failed to create TCP socket for new client: %s\n", strerror(errno));
        return -1;
    }

    return sock;
}


int send_connection_response(int udp_socket, struct sockaddr *client_info, struct query_datagramm *data)
{
    strncpy(data->msg, CONNECTION_UDP_RESPONSE, sizeof(CONNECTION_UDP_RESPONSE));
    if (sendto(udp_socket, data, 
                sizeof(struct query_datagramm), 0, 
                client_info, (socklen_t)sizeof(*client_info)) == -1) {
        fprintf(stderr, "Failed to send response to new client: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int create_client_connection(struct query_datagramm *data, client_connection *connections)
{
    int status, new_tcp_client_socket;
    struct addrinfo hints, *client_addr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;

    // Для нового клиента выхватываем первый попавшийся пустой порт
    if((status = getaddrinfo(data->address, NULL, &hints, &client_addr)) != 0) {
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
    freeaddrinfo(client_addr);

    return new_tcp_client_socket;
}
