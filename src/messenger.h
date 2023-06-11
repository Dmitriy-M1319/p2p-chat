#ifndef MESSENGER_H
#define MESSENGER_H


#include "connection_list.h"

#define MESSAGE_MAX_LENGTH 255

typedef struct client_message client_msg;

/**
 * Тип сообщения (для всех клиентов, для определенного клиента, для отключения от сети
 */
enum msg_type 
{
    ALL_CLIENTS_MSG,
    PRIVATE_MSG,
    UNCONNECT_MSG
};

/**
 * Отсоединиться ото всех участников чата
 * При ошибке возвращает -1
 */
int unconnect(client_connection *list, const char *client_name);

#endif
