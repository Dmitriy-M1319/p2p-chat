#ifndef UDP_CLIENT_CONNECTION_QUERY_H
#define UDP_CLIENT_CONNECTION_QUERY_H


#include <netinet/in.h>
#define UDP_NEW_CLIENT_PORT "55031" 
#define UDP_BROADCAST_PORT "55030"
#define BROADCAST_ADDR "192.168.0.255"
#define DATAGRAM_MSG_LENGTH 30

#define CONNECTION_UDP_REQUEST "request on connection"
#define CONNECTION_UDP_RESPONSE "responce on connection query"


/**
 * Структура данных, представляющая собой датаграмму, которая используется
 * при запросе клиента на включение в общую сеть
 */
struct query_datagramm
{
    char msg[DATAGRAM_MSG_LENGTH];
    int port;
};

/**
 * Создать сокет для широковещательной рассылки запроса на включение в сеть
 * Возвращает дескриптор сокета в случае успеха
 * Возвращает -1 в случае ошибки и выставляет errno
 */
int create_udb_broadcast_socket();

/**
 * Отправить запрос на широковещательный адрес сети для включения клиента в сеть
 * Возвращает -1 в случае ошибки
 */
int send_connection_query(int udp_socket);

/**
 * Создать новое TCP подключение для клиента, если поступил запрос на подключение в сеть, и делает его привязку к новому порту
 * Возвращает дескриптор сокета в случае успеха
 * В случае ошибки установления соединения возвращает -1
 */
int create_tcp_client_socket();

/**
 * Отправить информацию новому клиенту, что для него готов сокет и он может подключаться
 * В случае ошибки возвращается -1
 */
int send_connection_response(int udp_socket, struct sockaddr_in *client_info, int client_port);

#endif
