#include "ssl_utils.h"
#include <openssl/err.h>
#include <linux/limits.h>
#include <openssl/ssl.h>
#include <string.h>


SSL_CTX *get_context(enum ssl_context_type type)
{
    int result;
    const SSL_METHOD *protocol_method; 

    if (type == SSL_CONTEXT_FOR_CLIENT) {
        protocol_method= TLS_client_method();
    } else {
        protocol_method = TLS_server_method();
    }

    SSL_CTX *context = SSL_CTX_new(protocol_method);
    SSL_CTX_set_options(context, SSL_MODE_AUTO_RETRY);
    SSL_CTX_set_verify(context, SSL_VERIFY_PEER, NULL);

    if (type == SSL_CONTEXT_FOR_SERVER) {
        SSL_CTX_set_options(context, SSL_VERIFY_FAIL_IF_NO_PEER_CERT);
    }

    result = SSL_CTX_load_verify_locations(context, CA_CERTIFICATE_PATH, NULL);
    if (result != 1) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(context);
        return NULL;
    }

    result = SSL_CTX_use_certificate_file(context, CLIENT_SSL_CERTIFICATE_PATH, SSL_FILETYPE_PEM);
    if (result != 1) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(context);
        return NULL;
    }

    SSL_CTX_use_PrivateKey_file(context, CLIENT_SSL_PRIVATE_KEY_PATH, SSL_FILETYPE_PEM);
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

    SSL_CTX_set_verify_depth(context, 1);
    SSL_CTX_set_cipher_list(context, "HIGH:!aNULL:!kRSA:!PSK:!SRP:!MD5:!RC4");

    return context;
}



int check_server_certificate_sign(SSL_CTX *context, SSL *ssl_object)
{
    X509_STORE *certificates_store = SSL_CTX_get_cert_store(context);
    if (certificates_store== NULL) {
        fputs("failed to get certificates store\n", stderr);
        return -1;
    }

    X509 *server_certificate = SSL_get1_peer_certificate(ssl_object);
    if (server_certificate == NULL) {
        fputs("failed to get peer certificate to connection\n", stderr);
        return -1;
    }

    X509_STORE_CTX *contexts = X509_STORE_CTX_new();
    if (contexts == NULL) {
        fputs("failed to create new store context\n", stderr);
        X509_free(server_certificate);
        return -1;
    }

    if (X509_STORE_CTX_init(contexts, certificates_store, server_certificate, NULL) != 1) {
        fputs("failed to init new ctx\n", stderr);
        X509_STORE_CTX_free(contexts);
        X509_free(server_certificate);
        return -1;
    }

    if (X509_verify_cert(contexts) != 1) {
        fputs("ceritificate doesn't signed same ca certificate\n", stderr);
        X509_STORE_CTX_free(contexts);
        X509_free(server_certificate);
        return -1;
    }

    X509_STORE_CTX_free(contexts);
    X509_free(server_certificate);
    return 0;
}

void print_error(int err_value)
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

