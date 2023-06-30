#ifndef MESSENGER_H
#define MESSENGER_H


#include "connection_list.h"

#define MESSAGE_MAX_LENGTH 255

typedef struct client_message client_msg;

struct receive_msg_args
{
    client_connection *connections;
    client_connection *curr_client_connection;
};

/**
 * Тип сообщения (для всех клиентов, для определенного клиента, для файла для отключения от сети)
 */
enum msg_type 
{
    ALL_CLIENTS_MSG,
    PRIVATE_CLIENT_MSG,
    FILE_SEND_MSG,
    UNCONNECT_QUERY_MSG
};

struct client_message 
{
    enum msg_type type;
    char filename[MESSAGE_MAX_LENGTH];
    long filesize;
    char msg[MESSAGE_MAX_LENGTH];
};

/**
 * Отсоединиться ото всех участников чата
 * При ошибке возвращает -1
 */
int unconnect(client_connection *connections);

/**
 * Отправляет обычное текстовое сообщение всем или определенному пользователю
 * При ошибке возвращает -1
 */
int send_msg(client_connection *connections, const char *optional_client, const char *msg);

/**
 * Отправить файл определенному клиенту client
 * В случае ошибки возвращает -1
 */
int send_file_to_client(client_connection *connections, const char *filename, const char *client);

#endif
