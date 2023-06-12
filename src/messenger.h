#ifndef MESSENGER_H
#define MESSENGER_H


#include "connection_list.h"

#define MESSAGE_MAX_LENGTH 255

typedef struct client_message client_msg;

/**
 * Тип сообщения (для всех клиентов, для определенного клиента, для файла для отключения от сети)
 */
enum msg_type 
{
    ALL_CLIENTS_MSG,
    PRIVATE_MSG,
    FILE_MSG,
    UNCONNECT_MSG
};

/**
 * Отсоединиться ото всех участников чата
 * При ошибке возвращает -1
 */
int unconnect(client_connection *list);


/**
 * Отправляет обычное текстовое сообщение всем или определенному пользователю
 * При ошибке возвращает -1
 */
int send_msg(client_connection *list, const char *receiver, const char *msg);

/**
 * Отправить файл определенному клиенту receiver
 * В случае ошибки возвращает -1
 */
int send_file(client_connection *list, const char *filename, const char *receiver);

#endif
