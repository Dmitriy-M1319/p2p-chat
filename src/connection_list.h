/**
\file
\brief Заголовочный файл с описанием списка с клиентскими подключениями
\author Dmitriy Mamonov

Данный файл содержит в себе определение узла односвязного списка с описанием подключения к клиенту, 
а так же необходимые функции для работы c экземпляром списка
*/
#ifndef CONNECTIONS_LIST_H
#define CONNECTIONS_LIST_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/ssl.h>


/// Максимальная длина идентификатора клиента (никнейма)
#define CLIENT_NAME_MAX_LENGTH 256


/// Переопределение структуры узла подключения для внешнего использования
typedef struct client_connection_node client_connection;

/**
 * \brief Узел защищенного соединения с определенным клиентом в сети
 *
 * Структура, представляющая собой узел односвязного списка, которая хранит в себе TCP-сокет, связанный с другим клиентом,
 * сетевую информацию о присоединенном клиенте и объекты установки шифрования трафика
 */
struct client_connection_node
{
    char client_name[CLIENT_NAME_MAX_LENGTH];
    int client_socket;
    SSL *ssl_object;
    SSL_CTX *context;
    struct sockaddr_in client_address_info;
    struct client_connection_node *next;
};


/**
 * \brief Создать новый список подключений
 *
 * Создает новый пустой односвязный список c одним ложным подключением, которое используется в качестве указателя на начало списка
 * \return Указатель на первый элемент списка (ложное подключение) в случае успеха, NULL - в случае ошибки
 */
client_connection *create_connections();


/**
 * \brief Напечатать все подключения из списка
 * \param[in] list Список с клиентскими подключениями
 * 
 * Выводит в стандартный вывод все доступные подключения из односвязного списка
 *
 * Пример вывода подключения:
 * \code
 * Connection: Client_1
 * IPv4: 192.168.0.14:43560
 * \endcode
 */
void print_list(client_connection *list);


/**
 * \brief Создать новое защищенное соединение с клиентом
 * \param[out] connections Список с подключениями, куда будет добавлено новое соединение
 * \param[in] client Имя клиента (его идентификатор)
 * \param[in] socket TCP-сокет, через который будет идти взаимодействие с этим клиентом
 * \param[in] addr_info Сетевая информация о клиенте
 * \param[in] ssl_object Специальный объект SSL для шифрования трафика между клиентами
 * \param[in] context Специальный объект, хранящий конфигурацию защищенного подключения
 *
 * Добавить нового клиента и параметры соединения с ним в существующий список с подключениями
 *
 * \return Указатель на созданный узел, NULL - в случае ошибки
 */
client_connection *add_new_secure_connection(client_connection *connections, 
        const char *client, 
        const int socket, 
        const struct sockaddr_in *addr_info, 
        SSL *ssl_object, 
        SSL_CTX *context);


/**
 * \brief Получить информацию о соединении с определенным клиентом
 * \param[in] connections Список с подключениеми, откуда будет произведен поиск клиента
 * \param[in] client Имя искомого пользователя (его идентификатор)
 *
 * Получить параметры подключения из списка для определенного имени клиента
 * 
 * \return Указатель на узел соединения с клиентом, NULL - в случае ошибки
 */
client_connection *get_connection(client_connection *connections, const char *client);

/**
 * \brief Удалить соединение из списка
 * \param[out] connections Список с подключениеми, откуда будет произведено удаление клиента
 * \param[in] client Имя искомого пользователя (его идентификатор)
 *
 * Разрывает подключение с определенным клиентом и удаляет соответствующий ему узел из списка соединений
 * 
 * \return 0 - в случае успеха, -1 - в случае ошибки
 */
int remove_connection(client_connection *connections, const char *client);


/**
 * \brief Очистить весь список подключений
 * \param[out] connections_ptr Указатель на список подключений для очистки
 *
 * Закрывает подключения и удаляет все узлы подключений из списка
 * 
 * \return 0 - в случае успеха, -1 - в случае ошибки
 */
int free_connections(client_connection **connections_ptr);


#endif
