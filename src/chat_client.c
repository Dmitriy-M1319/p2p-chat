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
#define BROADCAST_ADDR "192.168.0.255"

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

    if((status = getaddrinfo(BROADCAST_ADDR, UDP_BROADCAST_PORT, &hints, &addr_res)) != 0) {
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

int main(int argc, char *argv[])
{
    int sock = create_udb_broadcast_socket();
    printf("Hello, I am future chat server...\n");
    close(sock);
    return 0;
}
