/**
\file
\brief Заголовочный файл с описанием вспомогательных функций для работы с SSL объектами
\author Dmitriy Mamonov

Данный файл содержит в себе определение перечисления типа SSL контекста, а также функций
для создания контекста, проверки сертификатов и печати ошибок
*/
#ifndef SSL_UTILS_H
#define SSL_UTILS_H

#include <openssl/ssl.h>
#include <openssl/types.h>

/// TODO: вынести в конфигурационный файл
#define CA_CERTIFICATE_PATH "ca.crt"
#define CLIENT_SSL_CERTIFICATE_PATH "chat.crt"
#define CLIENT_SSL_PRIVATE_KEY_PATH "chat.key"

/**
 * \brief Тип контекста SSL (сервер - клиент, который принимает новое подключение, 
 * или клиент - клиент, который запрашивает новое подключение)
 */
enum ssl_context_type
{
    SSL_CONTEXT_FOR_SERVER, ///< Контекст для серверного приложения
    SSL_CONTEXT_FOR_CLIENT ///< Контекст для клиентского приложения
};

/**
 * \brief Получить SSL контекст для клиента, который будет первым отправлять запрос на установку подключения
 * \param[in] type Вариант создания контекста
 *
 * \return Объект контекста SSL или NULL в случае ошибки
 */
SSL_CTX *get_context(enum ssl_context_type type);


/**
 * \brief Проверить сертификат клиента, с которым устанавливается соединение, на подпись корневым
 * \param[in] context Объект контекста SSL
 * \param[in] ssl_object Объект соединения SSL
 *
 * \return 0 - в случае успеха, -1 - в случае ошибки
 */
int check_server_certificate_sign(SSL_CTX *context, SSL *ssl_object);

/**
 * \brief Напечатать ошибку в ходе выполнения SSL-функций
 * \param[in] err_value Целочисленный код ошибки
 */
void print_error(int err_value);
#endif
