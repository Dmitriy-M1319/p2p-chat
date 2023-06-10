#include "udp_client_connection_query.h"
#include "connection_list.h"
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>

// Список всех подключений для клиентам в локальной сети
client_connection *connections = NULL;


void query_invitation_in_network(const char *client_name)
{
    int query_socket;
    struct sockaddr_in info;

    // Создаем сокет для широковещательного запроса в сеть
    if ((query_socket = create_udb_broadcast_socket()) < 0) {
        exit(1);
    }
    // Отправляем широковещательный запрос
    if (send_connection_query(query_socket, client_name) == -1) {
        exit(2);
    }

    struct query_datagramm data;
    struct sockaddr addr;
    socklen_t addr_len;
    int new_cl_socket;
    // Ждем получения ответа от других клиентов чата (походу это надо в отдельный поток вынести
    // как разделенное управление одним сокетом
    if (recvfrom(query_socket, &data, sizeof(data), 0, &addr, &addr_len) == -1) {
        fprintf(stderr, "failed to get connection_info from other client");
        exit(3);
    }
    
    connections = create_client_list();
    // Создаем новое подключение с клиентом (надо в отдельном потоке делать)
    if(create_client_connection(&data, connections) == -1) {
        fprintf(stderr, "Failed to create new connection to client\n");
        exit(3);
    }
}



void listen_new_clients(const char *client_name)
{
    // вот эта вещь тоже должна наверное висеть в отдельном потоке 
    int udp_listen_socket, status;
    struct addrinfo hints, *broadcast_addr;

    if ((udp_listen_socket = create_simple_udp_socket()) == -1) {
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    if ((status = getaddrinfo(BROADCAST_ADDR, UDP_BROADCAST_PORT, &hints, &broadcast_addr)) == -1) {
        fprintf(stderr, "Error with getaddrinfo: %s\n", gai_strerror(status));
        exit(2);
    } 

    if (bind(udp_listen_socket, broadcast_addr->ai_addr, broadcast_addr->ai_addrlen) == -1) {
        fprintf(stderr, "Failed to bind socket for client requests: %s\n", strerror(errno));
        exit(3);
    }

    freeaddrinfo(broadcast_addr);

    // Теперь ждем входящих запросов (пока ждем всегда, но потом надо будет забиндить выход из цикла на сигнал)
    while (1) {
        struct query_datagramm request; 
        int req_tcp_socket;
        if(recvfrom(udp_listen_socket, &request, sizeof(request), 0, NULL, NULL) == -1) {
            fprintf(stderr, "Failed to receive request from client: %s\n", strerror(errno));
            exit(1);
        }
        // Как только получили запрос, сразу же создаем все условия
        req_tcp_socket = create_tcp_client_socket();
        struct addrinfo *client_req;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = 0;
        if ((status = getaddrinfo(request.address, UDP_NEW_CLIENT_PORT, &hints, &client_req)) == -1) {
            fprintf(stderr, "Error with getaddrinfo: %s\n", gai_strerror(status));
            exit(2);
        }

        if (bind(req_tcp_socket, client_req->ai_addr, client_req->ai_addrlen) == -1) {
            fprintf(stderr, "Failed to bind new TCP socket for new client request: %s\n", strerror(errno));
            exit(1);
        }

        // Вот этот момент надо переработать TODO
        // Здесь нам надо отправить клиенту адрес и порт нового подключения
        send_connection_response(udp_listen_socket, (struct sockaddr_in *)client_req->ai_addr, atoi(UDP_NEW_CLIENT_PORT));
        listen(req_tcp_socket, 1);
        int new_tcp_sock = accept(req_tcp_socket, NULL, NULL);
        add_new_connection(connections, request.nickname, new_tcp_sock, (struct sockaddr_in *)client_req->ai_addr);
    }
    
}


int main(int argc, char *argv[])
{
    printf("Hello, I am future chat client...\n");
    printf("Send a msg on local broadcast..\n");
    return 0;
}
