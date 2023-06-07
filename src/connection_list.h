#ifndef CONNECTIONS_LIST_H
#define CONNECTIONS_LIST_H

#include <sys/socket.h>
#include <netinet/in.h>

#define CLIENT_NAME_MAX_LENGTH 256

/**
 * Структура, представляющая собой узел односвязного списка,
 * который хранит в себе TCP-сокет, связанный с другим клиентом,
 * и информацию о нем
 */
struct client_connection_node
{
    char client_name[CLIENT_NAME_MAX_LENGTH];
    int client_socket;
    struct sockaddr_in client_address_info;
    struct client_connection_node *next;
};

/**
 * Инициализирует первый узел односвязного списка
 * Возвращает -1 в результате ошибки иниациализации списка
 */
int initialize_client_list(struct client_connection_node *head);

/**
 * Добавить новое подключение в список подключений
 * Возвращает -1 в результате ошибки создания нового элемента
 */
int add_new_connection(struct client_connection_node *list, const char *name, int socket, const struct sockaddr_in *addr);

/**
 * Получить указатель на узел подключения для определенного имени клиента
 * Возвращает NULL в результате поиска узла в списке
 */
struct client_connection_node *get_client_info(struct client_connection_node *list, const char *name);

/**
 * Удалить определенное подключение из списка
 * Возвращает -1 в результате поиска узла в списке
 */
int remove_connection(struct client_connection_node *list, const char *name);

/**
 * Очищает весь список подключений к клиентам
 * Возвращает -1 в случае ошибки
 */
int free_connection_list(struct client_connection_node *list);


#endif
