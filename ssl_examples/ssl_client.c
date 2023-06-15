#include <openssl/types.h>
#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <assert.h>


void error(int err_value)
{
    switch (err_value) {
    case SSL_ERROR_NONE:
        break;
    case SSL_ERROR_SSL:
        printf("SSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
        break;
    case SSL_ERROR_WANT_READ:
        printf("Other app wants read\n");
        break;
    case SSL_ERROR_WANT_WRITE:
        printf("Other app wants write\n");
        break;
    case SSL_ERROR_SYSCALL:
        printf("System error: %s\n", strerror(errno));
        break;
    case SSL_ERROR_ZERO_RETURN:
        printf("Connection closed\n");
        break;
    case SSL_ERROR_WANT_CONNECT:
        printf("Other app wants connect\n");
        // необходимо завершить установку соединения
        break;
    case SSL_ERROR_WANT_ACCEPT:
        printf("Other app wants accept new connection\n");
        // необходимо завершить принятие соединения
        break;
    }
}


int main(void)
{
    int result;
    SSL_library_init();
    SSL_load_error_strings();
    SSL_CTX *context = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_options(context, SSL_MODE_AUTO_RETRY);

    result = SSL_CTX_load_verify_locations(context, "c_cert.crt", NULL);
    if (result != 1) {
        ERR_print_errors_fp(stderr);
        assert(0);
    }
    result = SSL_CTX_use_certificate_file(context, "client.crt", SSL_FILETYPE_PEM);
    if (result != 1) {
        ERR_print_errors_fp(stderr);
        assert(0);
    }
    puts("Загрузили сертификат");
    SSL_CTX_use_PrivateKey_file(context, "client.key", SSL_FILETYPE_PEM);
    if (result != 1) {
        ERR_print_errors_fp(stderr);
        assert(0);
    }
    puts("Загрузили ключ");
    SSL_CTX_check_private_key(context);
    puts("Проверили ключ");

    SSL *ssl = SSL_new(context);
    
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(23333);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        SSL_free(ssl);
        SSL_CTX_free(context);
        close(sock);
        return 1;
    }

    puts("connect");

    result = SSL_set_fd(ssl, sock);
    if (result != 1) {
        error(SSL_get_error(ssl, result));
    }
    puts("SSL set fd");
    result = SSL_connect(ssl);
    if (result != 1) {
        error(SSL_get_error(ssl, result));
    }
    puts("SSL connect");

    char buf[100];
    strncpy(buf, "hello world from ssl", sizeof(buf));

    int write_result;
    if ((write_result = SSL_write(ssl, buf, sizeof(buf))) <= 0) {
        error(SSL_get_error(ssl, write_result));
    }

    printf("Отправлено сообщение: %s\n", buf);

    SSL_free(ssl);
    SSL_CTX_free(context);
    close(sock);
    return 0;
}
