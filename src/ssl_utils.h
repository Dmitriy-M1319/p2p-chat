#ifndef SSL_UTILS_H
#define SSL_UTILS_H

#include <openssl/ssl.h>
#include <openssl/types.h>

//TODO: вынести в конфигурационный файл
#define CA_CERTIFICATE_PATH "ca.crt"
#define CLIENT_SSL_CERTIFICATE_PATH "chat.crt"
#define CLIENT_SSL_PRIVATE_KEY_PATH "chat.key"

/**
 * Тип контекста SSL (сервер - клиент, который принимает новое подключение, 
 * или клиент - клиент, который запрашивает новое подключение)
 */
enum ssl_context_type
{
    SSL_CONTEXT_FOR_SERVER,
    SSL_CONTEXT_FOR_CLIENT
};

/**
 * Получить SSL контекст для клиента, который будет первым отправлять запрос на установку подключения
 */
SSL_CTX *get_context(enum ssl_context_type type);

/**
 * Проверить сертификат клиента, с которым устанавливается соединение, на подпись корневым
 * В случае ошибки возвращает -1
 */
int check_server_certificate_sign(SSL_CTX *context, SSL *ssl_object);

void print_error(int err_value);
#endif
