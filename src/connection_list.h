#ifndef CONNECTIONS_LIST_H
#define CONNECTIONS_LIST_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/ssl.h>

#define CLIENT_NAME_MAX_LENGTH 256

/**
 * Структура, представляющая собой узел односвязного списка,
 * который хранит в себе TCP-сокет, связанный с другим клиентом,
 * и информацию о нем
 */

typedef struct client_connection_node client_connection;

struct client_connection_node
{
    char client_name[CLIENT_NAME_MAX_LENGTH];
    int client_socket;
    SSL *ssl;
    SSL_CTX *context;
    struct sockaddr_in client_address_info;
    struct client_connection_node *next;
};

/**
 * Создает новый пустой односвязный список
 * Возвращает -1 в результате ошибки иниациализации списка
 */
client_connection *create_client_list();

/**
 * Печатает список подключений в вывод
 */
void print_list(client_connection *list);

/**
 * Добавить новое подключение в список подключений
 * Возвращает ссылку на созданный объект
 */
client_connection *add_new_connection(client_connection *list, const char *name, int socket, const struct sockaddr_in *addr);
client_connection *add_new_secure_connection(client_connection *list, const char *name, int socket, const struct sockaddr_in *addr, SSL *ssl, SSL_CTX *ctx);

/**
 * Получить указатель на узел подключения для определенного имени клиента
 * Возвращает NULL в результате поиска узла в списке
 */
client_connection *get_client_info(client_connection *list, const char *name);

/**
 * Удалить определенное подключение из списка
 * Возвращает -1 в результате поиска узла в списке
 */
int remove_connection(client_connection *list, const char *name);

/**
 * Очищает весь список подключений к клиентам
 * Возвращает -1 в случае ошибки
 */
int free_connection_list(client_connection **list);


#endif
