#include "ssl_utils.h"
#include <openssl/err.h>
#include <linux/limits.h>
#include <openssl/ssl.h>
#include <string.h>


SSL_CTX *get_context(enum ssl_context_type type)
{
    int result;
    const SSL_METHOD *method; 

    if (type == SSL_CONTEXT_CLIENT) {
        method = TLS_client_method();
    } else {
        method = TLS_server_method();
    }

    SSL_CTX *context = SSL_CTX_new(method);
    SSL_CTX_set_options(context, SSL_MODE_AUTO_RETRY);
    SSL_CTX_set_verify(context, SSL_VERIFY_PEER, NULL);

    if (type == SSL_CONTEXT_SERVER) {
        SSL_CTX_set_options(context, SSL_VERIFY_FAIL_IF_NO_PEER_CERT);
    }

    result = SSL_CTX_load_verify_locations(context, CA_CERTIFICATE_PATH, NULL);
    if (result != 1) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(context);
        return NULL;
    }

    result = SSL_CTX_use_certificate_file(context, SSL_CERTIFICATE_PATH, SSL_FILETYPE_PEM);
    if (result != 1) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(context);
        return NULL;
    }

    SSL_CTX_use_PrivateKey_file(context, SSL_PKEY_PATH, SSL_FILETYPE_PEM);
    if (result != 1) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(context);
        return NULL;
    }

    if (!SSL_CTX_check_private_key(context)) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(context);
        return NULL;
    }
    return context;
}



int check_server_certificate_sign(SSL_CTX *context, SSL *ssl)
{
    X509_STORE *store = SSL_CTX_get_cert_store(context);
    if (store == NULL) {
        fputs("failed to get certificates store\n", stderr);
        return -1;
    }

    X509 *cert = SSL_get1_peer_certificate(ssl);
    if (cert == NULL) {
        fputs("failed to get peer certificate to connection\n", stderr);
        return -1;
    }

    X509_STORE_CTX *store_ctx = X509_STORE_CTX_new();
    if (store_ctx == NULL) {
        fputs("failed to create new store context\n", stderr);
        X509_free(cert);
        return -1;
    }

    if (X509_STORE_CTX_init(store_ctx, store, cert, NULL) != 1) {
        fputs("failed to init new ctx\n", stderr);
        X509_STORE_CTX_free(store_ctx);
        X509_free(cert);
        return -1;
    }

    int res = X509_verify_cert(store_ctx);
    if (res != 1) {
        fputs("ceritificate doesn't signed same ca certificate\n", stderr);
        X509_STORE_CTX_free(store_ctx);
        X509_free(cert);
        return -1;
    }

    X509_STORE_CTX_free(store_ctx);
    X509_free(cert);
    return 0;
}

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

