#include <openssl/types.h>
#include <stdio.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <stdlib.h>
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


void check_client_cert_sign(SSL_CTX *ctx, SSL *ssl)
{
    X509_STORE *store = SSL_CTX_get_cert_store(ctx);
    if (store == NULL) {
        fputs("failed to get_cert_store: \n", stderr);
    }

    X509 *cert = SSL_get0_peer_certificate(ssl);
    if (cert == NULL) {
        fputs("failed to get_peer_cert: \n", stderr);
    }

    X509_STORE_CTX *store_ctx = X509_STORE_CTX_new();
    if (store_ctx == NULL) {
        fputs("failed to create new ctx: \b", stderr);
    }

    if (X509_STORE_CTX_init(store_ctx, store, cert, NULL) != 1) {
        fputs("failed to init new ctx: ", stderr);
    }

    int res = X509_verify_cert(store_ctx);
    if (res != 1) {
        fputs("ceritificate doesn't signed same ca sert\n", stderr);
        X509_STORE_CTX_free(store_ctx);
        X509_free(cert);
    }

    X509_STORE_CTX_free(store_ctx);
    X509_free(cert);
}

int main(void)
{
    int result;
    SSL_library_init();
    SSL_load_error_strings();
    SSL_CTX *context = SSL_CTX_new(TLS_server_method());
    //SSL_CTX_set_verify(context, SSL_VERIFY_PEER, NULL);
    //SSL_CTX_set_options(context, SSL_VERIFY_FAIL_IF_NO_PEER_CERT);

    result = SSL_CTX_load_verify_locations(context, "c_cert.crt", NULL);
    if (result != 1) {
        ERR_print_errors_fp(stderr);
        assert(0);
    }
    result = SSL_CTX_use_certificate_file(context, "server.crt", SSL_FILETYPE_PEM);
    if (result != 1) {
        ERR_print_errors_fp(stderr);
        assert(0);
    }
    SSL_CTX_use_PrivateKey_file(context, "server.key", SSL_FILETYPE_PEM);
    if (result != 1) {
        ERR_print_errors_fp(stderr);
        assert(0);
    }
    SSL_CTX_check_private_key(context);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(23333);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    bind(sock, (struct sockaddr *)&addr, sizeof(addr));

    listen(sock, 1);

    int new_sock;
    if ((new_sock = accept(sock, NULL, NULL)) == -1) {
        perror("accept");
    };

    SSL *ssl = SSL_new(context);
    puts("SSL new object");

    result = SSL_set_fd(ssl, new_sock);

    printf("%d\n", result);
    puts("SSL set fd");
    if (result != 1) {
        error(SSL_get_error(ssl, result));
    }
    puts("SSL set fd success");
    result = 0;
    puts("SSL accept");
    sleep(2);
    while (result != 1) {
        result = SSL_accept(ssl);
        printf("result %d\n", result);
        if (result != 1) {
            error(SSL_get_error(ssl, result));
        }
    }
    check_client_cert_sign(context, ssl);
    puts("SSL accept success");

    char buf[100];

    int read_result;
    if ((read_result = SSL_read(ssl, buf, sizeof(buf))) <= 0) {
        error(SSL_get_error(ssl, read_result));
    }

    printf("Получено по защищенному соединению: %s\n", buf);

    int sd = SSL_get_fd(ssl);
    close(sd);
    SSL_free(ssl);
    SSL_CTX_free(context);
    close(sock);
    return 0;
}
