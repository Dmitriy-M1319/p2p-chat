#include "ssl_utils.h"
#include "udp_client_connection_query.h"
#include "connection_list.h"
#include "messenger.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>


// Список всех подключений для клиентов в локальной сети
client_connection *connections = NULL;
// Мьютекс для защиты списка подключений от работы нескольких потоков
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
// Мьютекс для вывода сообщений на экран пользователю
pthread_mutex_t print_mut = PTHREAD_MUTEX_INITIALIZER;


struct thread_args
{
    struct query_datagramm *data;
    client_connection *connection_list;
};


// Обработчик завершающего сигнала, который очищает все ресурсы программы и закрывает сокеты
void kill_program_handler(int sig)
{
    // Здесь сделать дополнительное отсоединение от других клиентов
    free_connection_list(connections);
    long max = sysconf(_SC_OPEN_MAX);

    while (--max >= 0)
        close(max);
    kill(0, SIGTERM);
}


void *receive_msg(void *data)
{
    struct receive_msg_args *args = (struct receive_msg_args *)data;
    int bytes_read;
    client_msg message; 
    char removed_user[CLIENT_NAME_MAX_LENGTH];
    char buffer[512] = {0};
    FILE *received_file_fd;
    long filesize, bytes_recv = 0;
    while (1) {
        memset(message.msg, 0, sizeof(message.msg));
        //if ((bytes_read = recv(args->curr_client->client_socket, &message, sizeof(message), 0)) < 0) {
        if ((bytes_read = SSL_read(args->curr_client->ssl, &message, sizeof(message))) <= 0) {
            fprintf(stderr, "Failed to receive message from client\n");
            ERR_print_errors_fp(stderr);
            return NULL;
        }
        switch (message.type) {
            case ALL_CLIENTS_MSG:
                pthread_mutex_lock(&print_mut);
                printf("Всем от %s: %s\n", args->curr_client->client_name, message.msg);
                pthread_mutex_unlock(&print_mut);
                break;
            case PRIVATE_MSG:
                pthread_mutex_lock(&print_mut);
                printf("Лично Вам от %s: %s\n", args->curr_client->client_name, message.msg);
                pthread_mutex_unlock(&print_mut);
                break;
            case UNCONNECT_MSG:
                strncpy(removed_user, args->curr_client->client_name, CLIENT_NAME_MAX_LENGTH);
                pthread_mutex_lock(&mut);
                remove_connection(args->list, args->curr_client->client_name);
                pthread_mutex_unlock(&mut);
                pthread_mutex_lock(&print_mut);
                printf("Пользователь %s покинул чат\n", removed_user);
                pthread_mutex_unlock(&print_mut);
                return NULL;
            case FILE_MSG:
                received_file_fd = fopen(message.filename, "wb");
                filesize = message.size;
                printf("От пользователя %s пришел запрос на отправку файла %s\n", args->curr_client->client_name, message.filename);
                while (bytes_recv < filesize) {
                    int read;
                    //if ((read = recv(args->curr_client->client_socket, &buffer, sizeof(buffer), 0)) < 0) {
                    if ((read = SSL_read(args->curr_client->ssl, &buffer, sizeof(buffer))) <= 0) {
                        fprintf(stderr, "Failed to receive a file data\n");
                        ERR_print_errors_fp(stderr);
                        break;
                    }
                    fwrite(buffer, 1, read, received_file_fd);
                    bytes_recv += read; 
                } 

                fclose(received_file_fd);
                printf("Пользователь %s отправил вам файл %s\n", args->curr_client->client_name, message.filename);
                memset(buffer, 0, sizeof(buffer));
                bytes_recv = 0;
                break;
        } 
    }
    return NULL;
}


void *thread_connection(void *args)
{
    struct thread_args *elements = (struct thread_args *)args;
    int s = pthread_mutex_lock(&mut);
    if (s != 0) {
        fprintf(stderr, "Failed to lock connection list\n");
        exit(1);
    }
    /*if(create_client_connection(elements->data, elements->connection_list) == -1) {
        fprintf(stderr, "Failed to create new connection to client\n");
        exit(2);
    }*/
    if(create_secure_connection(elements->data, elements->connection_list) == -1) {
        fprintf(stderr, "Failed to create new connection to client\n");
        exit(2);
    }
    pthread_mutex_unlock(&mut);
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
    socklen_t addr_len = sizeof(addr);
    connections = create_client_list();

    // Ждем получения ответа от других клиентов чата
    while(1) {
        if (recvfrom(query_socket, &data, sizeof(data), 0, &addr, &addr_len) == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // Ответа нет: или мы первые в сети, или подключились ко всем клиентам
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
    int udp_listen_socket, status;
    struct sockaddr_in broadcast_addr, local_addr;
    socklen_t length = sizeof(broadcast_addr);
    socklen_t local_length;
    struct addrinfo hints;

    if ((udp_listen_socket = create_simple_udp_socket()) == -1) {
        exit(1);
    }

    get_local_addr(&local_addr, &local_length);

    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(UDP_BROADCAST_PORT);
    inet_aton(BROADCAST_ADDR, &broadcast_addr.sin_addr);

    if (bind(udp_listen_socket, (struct sockaddr *)&broadcast_addr, length) == -1) {
        fprintf(stderr, "Failed to bind socket for client requests: %s\n", strerror(errno));
        exit(3);
    }


    // Теперь ждем входящих запросов (сделать завершение прослушивания через сигналы)
    while (1) {
        struct query_datagramm request; 
        int req_tcp_socket, ssl_result;
        struct sockaddr_in client_req;
        client_connection *new_client;
        socklen_t client_length = sizeof(client_req);
        if(recvfrom(udp_listen_socket, 
                    &request, sizeof(request), 0, 
                    (struct sockaddr *)&client_req, &client_length) == -1) {
            fprintf(stderr, "Failed to receive request from client: %s\n", strerror(errno));
            exit(1);
        }
        printf("Установка подключения к %s\n", request.nickname);
        // Как только получили запрос, сразу же создаем все условия
        
        SSL_CTX *context = get_context(SSL_CONTEXT_SERVER);
        SSL *ssl = SSL_new(context);
        req_tcp_socket = create_tcp_client_socket();

        // Привязываем новый сокет к локальному адресу
        if (bind(req_tcp_socket, (struct sockaddr *)&local_addr, local_length) == -1) {
            fprintf(stderr, "Failed to bind new TCP socket for new client request: %s\n", strerror(errno));
            exit(1);
        }

        struct query_datagramm response;
        struct sockaddr_in servaddr;
        socklen_t len = sizeof(servaddr);

        if(listen(req_tcp_socket, 1) < 0) {
            fprintf(stderr, "Failed to listen the TCP client socket: %s\n", strerror(errno));
            exit(1);
        }

        if (getsockname(req_tcp_socket, (struct sockaddr*)&servaddr, &len) < 0) {
            perror("getsockname error");
            exit(1);
        }

        response.port = ntohs(servaddr.sin_port); // порт, который был открыт для нового подключения
        strcpy(response.nickname, (char *)client_name);
        inet_ntop(local_addr.sin_family, &(local_addr.sin_addr), response.address, sizeof(response.address));

        send_connection_response(udp_listen_socket, (struct sockaddr *)&client_req, &response);
        int new_tcp_sock = accept(req_tcp_socket, NULL, NULL);

        ssl_result = SSL_set_fd(ssl, new_tcp_sock);
        if (ssl_result!= 1) {
            error(SSL_get_error(ssl, ssl_result));
            exit(1);
        }
        
        ssl_result = 0;
        sleep(2);
        while (ssl_result != 1) {
            ssl_result = SSL_accept(ssl);
            if (ssl_result != 1) {
                error(SSL_get_error(ssl, ssl_result));
                exit(3);
            }
        }


        /*if (!(new_client = add_new_connection(connections, request.nickname, new_tcp_sock, (struct sockaddr_in *)&client_req))) {
            fprintf(stderr, "Failed to add new client %s\n", request.nickname);
            exit(1);
        };*/

        if (!(new_client = add_new_secure_connection(connections, request.nickname, new_tcp_sock, (struct sockaddr_in *)&client_req, ssl, context))) {
            fprintf(stderr, "Failed to add new client %s\n", request.nickname);
            exit(1);
        }

        printf("Подключение к %s было установлено\n", request.nickname);
        pthread_t connection_th;
        struct receive_msg_args args;
        args.curr_client = new_client;
        args.list = connections;
        if (pthread_create(&connection_th, NULL, receive_msg, (void *)&args) < 0) {
            fprintf(stderr, "Failed to start new thread for connection with %s\n", request.nickname);
            exit(1);
        }
    }
   
    close(udp_listen_socket);
    return NULL;
}


void parse_chat_command(char *message)
{
    char delim[] = "|\"";
    char *user = NULL, *msg_or_filename;
    char *start = strtok(message, delim);
    if (strstr(start, ":send_msg")) {
        msg_or_filename = strtok(NULL, delim);
        pthread_mutex_lock(&print_mut);
        send_msg(connections, user, msg_or_filename);
        printf("Вы: %s\n", msg_or_filename);
        pthread_mutex_unlock(&print_mut);
    } else if (strstr(start, ":send_to")) {
        user = strtok(NULL, delim);
        msg_or_filename = strtok(NULL, delim);
        pthread_mutex_lock(&print_mut);
        send_msg(connections, user, msg_or_filename);
        printf("Вы: %s\n", msg_or_filename);
        pthread_mutex_unlock(&print_mut);
    } else if (strstr(start, ":send_file")) {
        user = strtok(NULL, delim);
        msg_or_filename = strtok(NULL, delim);
        pthread_mutex_lock(&print_mut);
        send_file(connections, msg_or_filename, user);
        printf("Отправлен файл: %s\n", msg_or_filename);
        pthread_mutex_unlock(&print_mut);
    } else {
        unconnect(connections);
        kill(getpid(), SIGINT);
    }
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

    if (signal(SIGINT, kill_program_handler) == SIG_ERR) {
        fprintf(stderr, "Failed to set new handler to signal kill program: %s\n" , strerror(errno));
        exit(1);
    }

    printf("Подключение к сети...\n");
    query_invitation_in_network(nickname);

    pthread_t listen_th;
    if (pthread_create(&listen_th, NULL, listen_new_clients, nickname) != 0) {
        fprintf(stderr, "Failed to create new thread for listening: %s\n" , strerror(errno));
        exit(1);
    }

    char comm[1024];
    int a = 0;
    while (1) {
        while((ch = getchar()) != EOF) {
            if (ch == '\n')
                break;
            comm[a] = (char)ch;
            ++a;
        }       
        parse_chat_command(comm);
        memset(comm, 0, 1024);
        a = 0;
    }

    void *res;
    pthread_join(listen_th, &res);
    printf("Listen thread return %ld\n", (long)res);

    free_connection_list(connections);
    return 0;
}
