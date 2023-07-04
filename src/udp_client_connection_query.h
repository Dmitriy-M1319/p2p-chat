/**
\file
\brief Заголовочный файл с описанием функциональности подключение клиента в общий чат
\author Dmitriy Mamonov

Данный файл содержит в себе определение пакета передачи данных для запроса на включение, 
а также функций для запроса на соединения и ответа на него, обработки и создания нового клиентского соединения
*/
#ifndef UDP_CLIENT_CONNECTION_QUERY_H
#define UDP_CLIENT_CONNECTION_QUERY_H


#include "connection_list.h"
#include <netinet/in.h>
#include <openssl/types.h>
// ----- Вот это все дело вынести в конфиг потом
#define UDP_BROADCAST_PORT 55030
#define BROADCAST_ADDR "255.255.255.255"
// ---------------------------------------------
/// Максимальная длина сообщения в запросе
#define QUERY_DATAGRAMM_MSG_LENGTH 30
/// Максимальная длина поля IPv4 адреса в запросе
#define QUERY_DATAGRAMM_ADDR_LENGTH 16
/// Максимальная длина поля имени (идентификатора) клиента в запросе
#define QUERY_DATAGRAMM_NICKNAME_LENGTH 255

#define NEW_CONNECTION_REQUEST_MESSAGE "request on connection"
#define NEW_CONNECTION_RESPONSE_MESSAGE "responce on connection query"


/**
 * \brief Пакет для запроса на вход в имеющийся чат
 *
 * Структура данных, представляющая собой датаграмму, которая используется
 * при запросе клиента на включение в общую сеть
 */
struct query_datagramm
{
    char msg[QUERY_DATAGRAMM_MSG_LENGTH];
    char address[QUERY_DATAGRAMM_ADDR_LENGTH];
    char nickname[QUERY_DATAGRAMM_NICKNAME_LENGTH];
    int port;
};


/**
 * \brief Создать широковещательный UDP-сокет
 *
 * Создает сокет для широковещательной рассылки запроса на включение в сеть
 *
 * \return Дескриптор сокета в случае успеха, -1 - в случае ошибки
 */
int create_udb_broadcast_socket();


/**
 * \brief Создать обычный UDP-сокет для приема ответа
 *
 * Создает сокет для отправки ответа новому клиенту, что он может быть приглашен
 *
 * \return Дескриптор сокета в случае успеха, -1 - в случае ошибки
 */
int create_response_udp_socket();


/**
 * \brief Отправить запрос на широковещательный адрес сети для включения клиента в сеть
 * \param[in] udp_socket UDP-сокет для отправки датаграммы с ответом клиенту
 * \param[in] nickname Имя (идентификатор) клиента-отправителя
 *
 * \return 0 - в случае успеха, -1 - в случае ошибки
 */
int send_connection_query(const int udp_socket, const char *nickname);


/**
 * \brief Получить адрес узла, на котором запускается клиент, в локальной сети
 * \param[out] address_out Структура, куда будет записана информация об локальном адресе в сети
 * \param[out] address_length Длина выходной структуры
 *
 * \return 0 - в случае успеха, -1 - в случае ошибки
 */
int get_local_address(struct sockaddr_in *address_out, socklen_t *address_length);


/**
 * \brief Создать сокет для ответившего клиента 
 *
 * Создать новое TCP подключение для клиента, если поступил запрос на подключение в сеть, и сделать его привязку к новому порту
 *
 * \return Дескриптор сокета в случае успеха, -1 - в случае ошибки
 */
int create_tcp_socket_for_client();


/**
 * \brief Отправить информацию новому клиенту, что для него готов сокет и он может подключаться
 * \param[in] udp_socket UDP-сокет для отправки датаграммы с ответом клиенту
 * \param[in] client_address Информация об адресе нового клиента
 * \param[in] response Пакет с ответом клиенту
 *
 * \return 0 - в случае успеха, -1 - в случае ошибки
 */
int send_connection_response(const int udp_socket, struct sockaddr *client_address, struct query_datagramm *response);


/**
 * \brief Создать новое защищенное TCP - подключение с клиентом, который ответил на запрос о включении в сеть
 * \param[in] data Датаграмма с ответом от старого клиента, которого необходимо добавить в соединения
 * \param[in] connections Список с подключениями нового клиента
 *
 * \return 0 - в случае успеха, -1 - в случае ошибки
 */
int create_secure_connection(struct query_datagramm *data, client_connection *connections);

#endif
