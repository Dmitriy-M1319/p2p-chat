/**
\file
\brief Заголовочный файл с описанием функциональности общения между клиентами
\author Dmitriy Mamonov

Данный файл содержит в себе определение пакета передачи данных между клиентами в чате, а также функций для отправки сообщения, файла и запроса на разрыв соединения
*/
#ifndef MESSENGER_H
#define MESSENGER_H


#include "connection_list.h"

/// Максимальная длина сообщения (или части файла) для передачи по сети
#define MESSAGE_MAX_LENGTH 255

/// Переопределение структуры пакета с клиентским сообщением для внешнего использования
typedef struct client_message client_msg;

struct receive_msg_args
{
    client_connection *connections;
    client_connection *curr_client_connection;
};

/// Тип сообщения от клиента
enum msg_type 
{
    ALL_CLIENTS_MSG, ///< Сообщение для всех участников чата
    PRIVATE_CLIENT_MSG, ///< Личное сообщение для определенного участника в чате
    FILE_SEND_MSG, ///< Запрос на отправку файла участнику чата
    UNCONNECT_QUERY_MSG ///< Запрос на разрыв соединения со всеми участниками
};

/**
 * \brief Пакет для передачи данных по сети между клиентами
 *
 * Структура, представляющая собой протокол передачи данных между клиентами: тип сообщения, возможное название файла, длина возможного файла
 * и само сообщение (или буфер для хранения байтов файла)
 */
struct client_message 
{
    enum msg_type type;
    char filename[MESSAGE_MAX_LENGTH];
    long filesize;
    char msg[MESSAGE_MAX_LENGTH];
};


/**
 * \brief Отсоединиться от чата
 * \param[out] connections Список подключенных на данный момент клиентов
 *
 * Отправляет запрос на отсоединение всем подключенным клиентам к текущему клиенту
 * 
 * \return 0 - в случае успеха, -1 - в случае ошибки
 */
int unconnect(client_connection *connections);


/**
 * \brief Отправить обычное сообщение
 * \param[in] connections Список подключенных на данный момент клиентов
 * \param[in] optional_client Имя (идентификатор) клиента для отправки личного сообщения (необязателен)
 * \param[in] msg Сообщение для отправки
 * 
 * Отправляет обычное текстовое сообщение всем или определенному пользователю
 *
 * \return 0 - в случае успеха, -1 - в случае ошибки
 */
int send_msg(client_connection *connections, const char *optional_client, const char *msg);

/**
 * \brief Отправить файл определенному клиенту
 * \param[in] connections Список подключенных на данный момент клиентов
 * \param[in] filename Путь до отправляемого файла
 * \param[in] client Имя (идентификатор) клиента для отправки
 * 
 * Отправляет файл клиенту, который сохраняется по тому же пути, который был у отправителя
 *
 * \return 0 - в случае успеха, -1 - в случае ошибки
 */
int send_file_to_client(client_connection *connections, const char *filename, const char *client);

#endif
