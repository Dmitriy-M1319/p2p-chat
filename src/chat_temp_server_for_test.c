#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#define UDP_BROADCAST_PORT "55030"

/**
 * Создать сокет для широковещательной рассылки запроса на включение в сеть
 */
int create_udb_broadcast_socket() {
    int sock, status;
    struct addrinfo hints, *addr_res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE;

    if((status = getaddrinfo(NULL, UDP_BROADCAST_PORT, &hints, &addr_res)) != 0) {
        fprintf(stderr, "Error with getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    if((sock = socket(addr_res->ai_family, addr_res->ai_socktype, addr_res->ai_protocol)) == -1) {
        fprintf(stderr, "Failed to create socket for invitation in network: %s\n", strerror(errno));
        exit(2);
    }

    if((status = bind(sock, addr_res->ai_addr, addr_res->ai_addrlen)) == -1) {
        fprintf(stderr, "Failed to bind socket with port for invitation in network: %s\n", strerror(errno));
        exit(2);
    }

    freeaddrinfo(addr_res);
    return sock;
}

void recv_data_from_test_client(int sock_udp) {
    char buf[1024];
    int status;
    struct sockaddr_in result;
    socklen_t res_len = sizeof(result);

    if(recvfrom(sock_udp, buf, 1024, 0, (struct sockaddr *)&result, &res_len) == -1) {
        fprintf(stderr, "Failed to receive data from chat client: %s\n", strerror(errno));
        exit(2);
    }

    printf("Message from client: %s\n", buf);
}

int main(int argc, char *argv[])
{
    int sock = create_udb_broadcast_socket();
    printf("Hello, I am test chat server...\n");
    printf("Listen a msg from local client..\n");
    recv_data_from_test_client(sock);
    close(sock);
    return 0;
}
