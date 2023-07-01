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
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>


// Список всех подключений для клиентов в локальной сети
client_connection *connections = NULL;
// Мьютекс для защиты списка подключений от работы нескольких потоков
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// Мьютекс для вывода сообщений на экран пользователю
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;


struct thread_args
{
    struct query_datagramm *data;
    client_connection *connection_list;
};


// Обработчик завершающего сигнала, который очищает все ресурсы программы и закрывает сокеты
void kill_program_handler(int sig)
{
    // Здесь сделать дополнительное отсоединение от других клиентов
    free_connections(&connections);
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
    char removed_client[CLIENT_NAME_MAX_LENGTH];
    char data_buffer[512] = {0};
    FILE *received_file_descriptor;
    long filesize, bytes_received = 0;
    while (1) {
        memset(message.msg, 0, sizeof(message.msg));
        if ((bytes_read = SSL_read(args->curr_client_connection->ssl_object, &message, sizeof(message))) <= 0) {
            fprintf(stderr, "Failed to receive message from client\n");
            ERR_print_errors_fp(stderr);
            return NULL;
        }
        switch (message.type) {
            case ALL_CLIENTS_MSG:
                pthread_mutex_lock(&print_mutex);
                printf("Всем от %s: %s\n", args->curr_client_connection->client_name, message.msg);
                pthread_mutex_unlock(&print_mutex);
                break;
            case PRIVATE_CLIENT_MSG:
                pthread_mutex_lock(&print_mutex);
                printf("Лично Вам от %s: %s\n", args->curr_client_connection->client_name, message.msg);
                pthread_mutex_unlock(&print_mutex);
                break;
            case UNCONNECT_QUERY_MSG:
                strncpy(removed_client, args->curr_client_connection->client_name, CLIENT_NAME_MAX_LENGTH);
                pthread_mutex_lock(&mutex);
                remove_connection(args->connections, args->curr_client_connection->client_name);
                pthread_mutex_unlock(&mutex);
                pthread_mutex_lock(&print_mutex);
                printf("Пользователь %s покинул чат\n", removed_client);
                pthread_mutex_unlock(&print_mutex);
                return NULL;
            case FILE_SEND_MSG:
                received_file_descriptor = fopen(message.filename, "wb");
                filesize = message.filesize;
                printf("От пользователя %s пришел запрос на отправку файла %s\n", args->curr_client_connection->client_name, message.filename);
                while (bytes_received < filesize) {
                    if ((bytes_read = SSL_read(args->curr_client_connection->ssl_object, &data_buffer, sizeof(data_buffer))) <= 0) {
                        fprintf(stderr, "Failed to receive a file data\n");
                        ERR_print_errors_fp(stderr);
                        break;
                    }
                    fwrite(data_buffer, 1, bytes_read, received_file_descriptor);
                    bytes_received += bytes_read; 
                } 

                fclose(received_file_descriptor);
                printf("Пользователь %s отправил вам файл %s\n", args->curr_client_connection->client_name, message.filename);
                memset(data_buffer, 0, sizeof(data_buffer));
                bytes_received = 0;
                break;
        } 
    }
    return NULL;
}


void *create_connection_in_thread(void *args)
{
    struct thread_args *elements = (struct thread_args *)args;
    if (pthread_mutex_lock(&mutex) != 0) {
        fprintf(stderr, "Failed to lock connection list\n");
        exit(1);
    }
    if(create_secure_connection(elements->data, elements->connection_list) == -1) {
        fprintf(stderr, "Failed to create new connection to client\n");
        exit(2);
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}


void invite_in_network(const char *client_name)
{
    int query_socket;
    if ((query_socket = create_udb_broadcast_socket()) < 0) {
        exit(1);
    }
    if (send_connection_query(query_socket, client_name) == -1) {
        exit(2);
    }

    struct query_datagramm response;
    struct sockaddr old_client_address;
    socklen_t address_len = sizeof(old_client_address);
    connections = create_connections();

    while(1) {
        if (recvfrom(query_socket, &response, sizeof(response), 0, &old_client_address, &address_len) == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                close(query_socket);
                break;
            }
            else {
                fprintf(stderr, "failed to get connection_info from other client");
                exit(3);
            }
        }

        pthread_t new_thread;
        struct thread_args args = {&response, connections};

        if (pthread_create(&new_thread, NULL, create_connection_in_thread, &args) != 0) {
            fprintf(stderr, "Failed to create a new thread for create connection: %s\n", strerror(errno));
            exit(1);
        } 
    }
}


void *listen_new_clients_connections(void *my_name)
{
    int udp_listen_socket, status;
    struct sockaddr_in broadcast_address, my_address;
    socklen_t broadcast_length = sizeof(broadcast_address);
    socklen_t local_length;
    struct addrinfo hints;
    if ((udp_listen_socket = create_response_udp_socket()) == -1) {
        exit(1);
    }
    get_local_address(&my_address, &local_length);
    broadcast_address.sin_family = AF_INET;
    broadcast_address.sin_port = htons(UDP_BROADCAST_PORT);
    inet_aton(BROADCAST_ADDR, &broadcast_address.sin_addr);

    if (bind(udp_listen_socket, (struct sockaddr *)&broadcast_address, broadcast_length) == -1) {
        fprintf(stderr, "Failed to bind socket for client requests: %s\n", strerror(errno));
        exit(3);
    }

    while (1) {
        struct query_datagramm request; 
        int request_socket, ssl_result;
        struct sockaddr_in new_client_address;
        client_connection *new_client;
        socklen_t client_length = sizeof(new_client_address);
        if(recvfrom(udp_listen_socket, 
                    &request, sizeof(request), 0, 
                    (struct sockaddr *)&new_client_address, &client_length) == -1) {
            fprintf(stderr, "Failed to receive request from client: %s\n", strerror(errno));
            exit(1);
        }
        printf("Установка подключения к %s\n", request.nickname);
        SSL_CTX *context = get_context(SSL_CONTEXT_FOR_SERVER);
        SSL *ssl = SSL_new(context);
        request_socket = create_tcp_socket_for_client();

        if (bind(request_socket, (struct sockaddr *)&my_address, local_length) == -1) {
            fprintf(stderr, "Failed to bind new TCP socket for new client request: %s\n", strerror(errno));
            exit(1);
        }

        struct query_datagramm response;
        struct sockaddr_in servaddr;
        socklen_t len = sizeof(servaddr);

        if(listen(request_socket, 1) < 0) {
            fprintf(stderr, "Failed to listen the TCP client socket: %s\n", strerror(errno));
            exit(1);
        }

        if (getsockname(request_socket, (struct sockaddr*)&servaddr, &len) < 0) {
            perror("getsockname error");
            exit(1);
        }

        response.port = ntohs(servaddr.sin_port);
        strcpy(response.nickname, (char *)my_name);
        inet_ntop(my_address.sin_family, &(my_address.sin_addr), response.address, sizeof(response.address));

        send_connection_response(udp_listen_socket, (struct sockaddr *)&new_client_address, &response);
        int accepted_request_socket = accept(request_socket, NULL, NULL);

        ssl_result = SSL_set_fd(ssl, accepted_request_socket);
        if (ssl_result!= 1) {
            print_error(SSL_get_error(ssl, ssl_result));
            exit(1);
        }
        
        ssl_result = 0;
        sleep(2);
        while (ssl_result != 1) {
            ssl_result = SSL_accept(ssl);
            if (ssl_result != 1) {
                print_error(SSL_get_error(ssl, ssl_result));
                exit(3);
            }
        }
        if (!(new_client = add_new_secure_connection(connections, 
                        request.nickname, 
                        accepted_request_socket, 
                        (struct sockaddr_in *)&new_client_address, 
                        ssl, context))) {
            fprintf(stderr, "Failed to add new client %s\n", request.nickname);
            exit(1);
        }
        printf("Подключение к %s было установлено\n", request.nickname);
        pthread_t other_connection;
        struct receive_msg_args args;
        args.curr_client_connection = new_client;
        args.connections = connections;
        if (pthread_create(&other_connection, NULL, receive_msg, (void *)&args) < 0) {
            fprintf(stderr, "Failed to start new thread for connection with %s\n", request.nickname);
            exit(1);
        }
    }
    close(udp_listen_socket);
    return NULL;
}


void parse_chat_command(char *message)
{
    char delimiters[] = "|\"";
    char *receiver = NULL, *msg_or_filename;
    char *start = strtok(message, delimiters);
    if (strstr(start, ":send_msg")) {
        msg_or_filename = strtok(NULL, delimiters);
        pthread_mutex_lock(&print_mutex);
        send_msg(connections, receiver, msg_or_filename);
        printf("Вы: %s\n", msg_or_filename);
        pthread_mutex_unlock(&print_mutex);
    } else if (strstr(start, ":send_to")) {
        receiver = strtok(NULL, delimiters);
        msg_or_filename = strtok(NULL, delimiters);
        pthread_mutex_lock(&print_mutex);
        send_msg(connections, receiver, msg_or_filename);
        printf("Вы: %s\n", msg_or_filename);
        pthread_mutex_unlock(&print_mutex);
    } else if (strstr(start, ":send_file")) {
        receiver = strtok(NULL, delimiters);
        msg_or_filename = strtok(NULL, delimiters);
        pthread_mutex_lock(&print_mutex);
        send_file_to_client(connections, msg_or_filename, receiver);
        printf("Отправлен файл: %s\n", msg_or_filename);
        pthread_mutex_unlock(&print_mutex);
    } else {
        unconnect(connections);
        kill(getpid(), SIGINT);
    }
}


int main(int argc, char *argv[])
{
    char nickname[QUERY_DATAGRAMM_NICKNAME_LENGTH];
    int stdin_character;
    int i = 0;
    printf("Введите имя, под которым вы будете видны остальным участникам: ");
    while((stdin_character = getchar()) != EOF) {
        if (stdin_character == '\n')
            break;
        nickname[i] = (char)stdin_character;
        ++i;
    } 

    if (signal(SIGINT, kill_program_handler) == SIG_ERR) {
        fprintf(stderr, "Failed to set new handler to signal kill program: %s\n" , strerror(errno));
        exit(1);
    }

    printf("Подключение к сети...\n");
    invite_in_network(nickname);

    pthread_t listen_th;
    if (pthread_create(&listen_th, NULL, listen_new_clients_connections, nickname) != 0) {
        fprintf(stderr, "Failed to create new thread for listening: %s\n" , strerror(errno));
        exit(1);
    }

    char chat_command[1024];
    int a = 0;
    while (1) {
        while((stdin_character = getchar()) != EOF) {
            if (stdin_character == '\n')
                break;
            chat_command[a] = (char)stdin_character;
            ++a;
        }       
        parse_chat_command(chat_command);
        memset(chat_command, 0, 1024);
        a = 0;
    }

    void *result;
    pthread_join(listen_th, &result);
    printf("Listen thread return %ld\n", (long)result);

    free_connections(&connections);
    return 0;
}
