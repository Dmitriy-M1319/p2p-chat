#ifndef UDP_CLIENT_CONNECTION_QUERY_H
#define UDP_CLIENT_CONNECTION_QUERY_H


#include "connection_list.h"
#include <netinet/in.h>
// ----- Вот это все дело вынести в конфиг потом
#define UDP_NEW_CLIENT_PORT "55031" 
#define UDP_BROADCAST_PORT 55030
#define BROADCAST_ADDR "255.255.255.255"
// ---------------------------------------------
#define DATAGRAM_MSG_LENGTH 30
#define DATAGRAM_ADDR_LENGTH 16
#define DATAGRAM_NICKNAME_LENGTH 255

#define CONNECTION_UDP_REQUEST "request on connection"
#define CONNECTION_UDP_RESPONSE "responce on connection query"


/**
 * Структура данных, представляющая собой датаграмму, которая используется
 * при запросе клиента на включение в общую сеть
 */
struct query_datagramm
{
    char msg[DATAGRAM_MSG_LENGTH];
    char address[DATAGRAM_ADDR_LENGTH];
    char nickname[DATAGRAM_NICKNAME_LENGTH];
    int port;
};

/**
 * Создать сокет для широковещательной рассылки запроса на включение в сеть
 * Возвращает дескриптор сокета в случае успеха
 * Возвращает -1 в случае ошибки и выставляет errno
 */
int create_udb_broadcast_socket();

/**
 * Создать сокет для отправки ответа новому клиенту, что он может быть приглашен
 * Возвращает дескриптор сокета в случае успеха
 * Возвращает -1 в случае ошибки и выставляет errno
 */
int create_simple_udp_socket();

/**
 * Отправить запрос на широковещательный адрес сети для включения клиента в сеть
 * Возвращает -1 в случае ошибки
 */
int send_connection_query(int udp_socket, const char *local_nickname);

/**
 * Получает адрес узла, на котором запускается клиент, в локальной сети
 */
int get_local_addr(struct sockaddr_in *addr_out, socklen_t *length);

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
int send_connection_response(int udp_socket, struct sockaddr *client_info, struct query_datagramm *data);

/**
 * Создать новое TCP - подключение с клиентом, который ответил на запрос о включении в сеть
 * В случае ошибки возвращает -1
 */
int create_client_connection(struct query_datagramm *data, client_connection *connections);
#endif
