#include "udp_client_connection_query.h"
#include "connection_list.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>


// Список всех подключений для клиентов в локальной сети
client_connection *connections = NULL;
// Мьютекс для защиты списка подключений от работы нескольких потоков
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

struct thread_args
{
    struct query_datagramm *data;
    client_connection *connection_list;
};

void *thread_connection(void *args)
{
    struct thread_args *elements = (struct thread_args *)args;
    int s = pthread_mutex_lock(&mut);
    if (s != 0) {
        fprintf(stderr, "Failed to lock connection list\n");
        exit(1);
    }
    if(create_client_connection(elements->data, elements->connection_list) == -1) {
        fprintf(stderr, "Failed to create new connection to client\n");
        exit(2);
    }
    return NULL;
}

void query_invitation_in_network(const char *client_name)
{
    int query_socket;
    // Создаем сокет для широковещательного запроса в сеть
    if ((query_socket = create_udb_broadcast_socket()) < 0) {
        exit(1);
    }
    puts(client_name);
    // Отправляем широковещательный запрос
    if (send_connection_query(query_socket, client_name) == -1) {
        exit(2);
    }

    struct query_datagramm data;
    struct sockaddr addr;
    socklen_t addr_len;
    connections = create_client_list();

    // Ждем получения ответа от других клиентов чата
    while(1) {
        if (recvfrom(query_socket, &data, sizeof(data), 0, &addr, &addr_len) == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // Ответа нет: или мы первые в сети, или подключились ко всем клиентам
                puts("Кто то в сети первый");
                close(query_socket);
                break;
            }
            else {
                fprintf(stderr, "failed to get connection_info from other client");
                exit(3);
            }
        }

        pthread_t new_thread;
        struct thread_args args = { &data, connections };

        if (pthread_create(&new_thread, NULL, thread_connection, &args) != 0) {
            fprintf(stderr, "Failed to create a new thread for create connection: %s\n", strerror(errno));
            exit(1);
        } 
    }
}


void *listen_new_clients(void *client_name)
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


    // Теперь ждем входящих запросов (сделать завершение прослушивания через сигналы)
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

        // Здесь тоже выделяем первый попавшийся незанятый порт для клиента
        if ((status = getaddrinfo(request.address, NULL, &hints, &client_req)) == -1) {
            fprintf(stderr, "Error with getaddrinfo: %s\n", gai_strerror(status));
            exit(2);
        }

        if (bind(req_tcp_socket, client_req->ai_addr, client_req->ai_addrlen) == -1) {
            fprintf(stderr, "Failed to bind new TCP socket for new client request: %s\n", strerror(errno));
            exit(1);
        }

        struct sockaddr_in me;
        socklen_t me_ln = sizeof(me);

        if (get_local_addr(&me, &me_ln) == -1) {
            exit(1);
        }

        struct query_datagramm response;
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)client_req;
        response.port = ipv4->sin_port;
        strcpy(response.nickname, (char *)client_name);
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(ipv4->sin_family, &ipv4->sin_addr, ip_str, sizeof(ip_str));

        listen(req_tcp_socket, 1);
        // Тут есть вероятность, что подключаемый клиент отправит запрос раньше, чем дело
        // дойдет до accept
        send_connection_response(udp_listen_socket, client_req->ai_addr, &response);
        int new_tcp_sock = accept(req_tcp_socket, NULL, NULL);
        add_new_connection(connections, request.nickname, new_tcp_sock, (struct sockaddr_in *)client_req->ai_addr);
        freeaddrinfo(client_req);
        close(req_tcp_socket);
    }
   
    close(udp_listen_socket);
    freeaddrinfo(broadcast_addr);
    return NULL;
}


int main(int argc, char *argv[])
{
    char nickname[DATAGRAM_NICKNAME_LENGTH];
    int ch;
    int i = 0;
    printf("Hello, I am future chat client...\n");
    printf("Введите имя, под которым вы будете видны остальным участникам: ");
    while((ch = getchar()) != EOF) {
        if (ch == '\n')
            break;
        nickname[i] = (char)ch;
        ++i;
    } 
    printf("Подключение к сети...\n");
    query_invitation_in_network(nickname);

    // Отдельный поток
    pthread_t listen_th;
    if (pthread_create(&listen_th, NULL, listen_new_clients, nickname) != 0) {
        fprintf(stderr, "Failed to create new thread for listening: %s\n" , strerror(errno));
        exit(1);
    }

    void *res;
    pthread_join(listen_th, &res);
    printf("Listen thread return %ld\n", (long)res);

    free_connection_list(connections);
    return 0;
}
