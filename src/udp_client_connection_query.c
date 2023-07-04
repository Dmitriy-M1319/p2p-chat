#include "udp_client_connection_query.h"
#include "connection_list.h"
#include "ssl_utils.h"
#include <netinet/in.h>
#include <openssl/ssl.h>
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

#define LOCAL_NETWORK_FIRST_OCTET "192"

int create_udb_broadcast_socket()
{
    int udp_socket;
    if((udp_socket = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Failed to create socket for invitation in network: %s\n", strerror(errno));
        return -1;
    }
    int broadcast_option = 1;
    if(setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, &broadcast_option, sizeof(broadcast_option)) == -1) {
        fprintf(stderr, "Failed to set socket for broadcast for invitation in network: %s\n", strerror(errno));
        return -1;
    }
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt");
        return -1;
    }
    return udp_socket;
}


int create_simple_udp_socket()
{
    int udp_socket;
    if((udp_socket = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Failed to create socket for response to client: %s\n", strerror(errno));
        return -1;
    }
    return udp_socket;
}


int send_connection_query(const int udp_socket, const char *nickname)
{
    struct sockaddr_in local_address, broadcast_address;
    socklen_t address_length;

    if (get_local_address(&local_address, &address_length) == -1) {
        return -1;
    }

    broadcast_address.sin_family = AF_INET;
    broadcast_address.sin_port = htons(UDP_BROADCAST_PORT);
    inet_aton(BROADCAST_ADDR, &(broadcast_address.sin_addr));
    address_length = sizeof(broadcast_address);

    struct query_datagramm request;
    request.port = 0;
    strncpy(request.msg, NEW_CONNECTION_REQUEST_MESSAGE, sizeof(NEW_CONNECTION_REQUEST_MESSAGE));
    strncpy(request.nickname, nickname, QUERY_DATAGRAMM_NICKNAME_LENGTH);
    inet_ntop(local_address.sin_family, &(local_address.sin_addr), request.address, sizeof(request.address));

    if (sendto(udp_socket, &request, sizeof(request), 0, (struct sockaddr *)&broadcast_address, address_length) == -1) {
        fprintf(stderr, "Failed to send query for invitation in network: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}


int get_local_address(struct sockaddr_in *address_out, socklen_t *address_length)
{
    char hostname[1024];
    int status;
    struct addrinfo hints, *local_address;
    if(gethostname(hostname, 1024) == -1){
        fprintf(stderr, "Failed to get hostname for client machine: %s\n", strerror(errno));
        return -1;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    if ((status = getaddrinfo(hostname, NULL, &hints, &local_address)) == -1) {
        fprintf(stderr, "Error with getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }
    char ip_address_str[INET_ADDRSTRLEN];
    for (struct addrinfo *p = local_address; p != NULL; p = p->ai_next) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        struct in_addr *addr = &(ipv4->sin_addr);
        inet_ntop(p->ai_family, addr, ip_address_str, sizeof(ip_address_str));

        if (strstr(ip_address_str, LOCAL_NETWORK_FIRST_OCTET)) {
            memcpy(address_out, ipv4, sizeof(struct sockaddr_in));
            memcpy(address_length, &p->ai_addrlen, sizeof(socklen_t));
            freeaddrinfo(local_address);
            return 0;
        }
    }
    freeaddrinfo(local_address);
    return -1;
}


int create_tcp_socket_for_client()
{
    int tcp_socket;
    if ((tcp_socket = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Failed to create TCP socket for new client: %s\n", strerror(errno));
        return -1;
    }
    return tcp_socket;
}


int send_connection_response(const int udp_socket, struct sockaddr *client_address, struct query_datagramm *response)
{
    strncpy(response->msg, NEW_CONNECTION_RESPONSE_MESSAGE, sizeof(NEW_CONNECTION_RESPONSE_MESSAGE));
    if (sendto(udp_socket, response, 
                sizeof(struct query_datagramm), 0, 
                client_address, (socklen_t)sizeof(*client_address)) == -1) {
        fprintf(stderr, "Failed to send response to new client: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}


int create_secure_connection(struct query_datagramm *data, client_connection *connections)
{
    int status, new_client_socket;
    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);
    SSL_CTX *context;
    SSL *ssl;

    printf("Получен ответ на запрос подключения от %s\n", data->nickname);

    context = get_context(SSL_CONTEXT_FOR_CLIENT);
    ssl = SSL_new(context);

    new_client_socket = create_tcp_socket_for_client();
    client_addr.sin_port = htons(data->port);
    client_addr.sin_family = AF_INET;
    inet_aton(data->address, &(client_addr.sin_addr));

    sleep(1);
    if(connect(new_client_socket, (struct sockaddr *)&client_addr, length) < 0) {
        fprintf(stderr, "Failed to connect to new client: %s\n", strerror(errno));
        SSL_free(ssl);
        SSL_CTX_free(context);
        return -1;
    }

    status = SSL_set_fd(ssl, new_client_socket);
    if (status != 1) {
        print_error(SSL_get_error(ssl, status));
    }
    status = SSL_connect(ssl);
    if (status != 1) {
        print_error(SSL_get_error(ssl, status));
    }

    check_server_certificate_sign(context, ssl);
    sleep(2);

    if(!add_new_secure_connection(connections, data->nickname, new_client_socket, &client_addr, ssl, context)) {
        fprintf(stderr, "Failed to add new client %s\n", data->nickname);
        return -1;
    }

    printf("Клиент %s успешно подключен", data->nickname);
    return 0;
}
