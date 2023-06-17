#include "../src/connection_list.h"
#include "../src/ssl_utils.h"
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_create_list()
{
    client_connection *list = create_client_list();
    puts("list is created"); 
    int result = free_connection_list(&list);
    puts("list is destroyed"); 
    assert(result != -1);
    assert(list == NULL);
}

void test_common_add_new_connection()
{
    client_connection *list = create_client_list();
    puts("list is created"); 
    int result;
    char name[] = "test connection";
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(23334);
    addr.sin_family = AF_INET;
    client_connection *node = add_new_connection(list, name, sock, &addr);
    assert(node != NULL);
    assert(strncmp(node->client_name, name, CLIENT_NAME_MAX_LENGTH) == 0);
    result = free_connection_list(&list);
    assert(result != -1);
    assert(list == NULL);
}

void test_common_add_new_secure_connection()
{
    client_connection *list = create_client_list();
    SSL_CTX *context = get_context(SSL_CONTEXT_CLIENT);
    SSL *ssl = SSL_new(context);
    puts("list is created"); 
    int result;
    char name[] = "test connection";
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(23333);
    addr.sin_family = AF_INET;
    client_connection *node = add_new_secure_connection(list, name, sock, &addr, ssl, context);
    print_list(list);
    assert(node != NULL);
    assert(strncmp(node->client_name, name, CLIENT_NAME_MAX_LENGTH) == 0);
    assert(node->ssl != NULL);
    assert(node->context != NULL);
    result = free_connection_list(&list);
    assert(result != -1);
    assert(list == NULL);
}

void test_common_get_client_info()
{
    client_connection *list = create_client_list();
    puts("list is created"); 
    client_connection *finded;
    char name[] = "test connection";
    finded = get_client_info(list, name);
    assert(finded == NULL);

    int result;
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(23334);
    addr.sin_family = AF_INET;
    client_connection *node = add_new_connection(list, name, sock, &addr);
    print_list(list);
    assert(node != NULL);
    assert(strncmp(node->client_name, name, CLIENT_NAME_MAX_LENGTH) == 0);

    finded = get_client_info(list, name);
    assert(finded != NULL);
    assert(strncmp(finded->client_name, name, CLIENT_NAME_MAX_LENGTH) == 0);

    result = free_connection_list(&list);
    assert(result != -1);
    assert(list == NULL);
}

void test_common_remove_connection()
{
    client_connection *list = create_client_list();
    puts("list is created"); 
    client_connection *finded;
    char name[] = "test connection";
    finded = get_client_info(list, name);
    assert(finded == NULL);

    int result;
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(23334);
    addr.sin_family = AF_INET;
    client_connection *node = add_new_connection(list, name, sock, &addr);
    print_list(list);
    assert(node != NULL);
    assert(strncmp(node->client_name, name, CLIENT_NAME_MAX_LENGTH) == 0);

    finded = get_client_info(list, name);
    assert(finded != NULL);
    assert(strncmp(finded->client_name, name, CLIENT_NAME_MAX_LENGTH) == 0);

    result = remove_connection(list, name);
    assert(result != -1);
    finded = get_client_info(list, name);
    assert(finded == NULL);

    result = free_connection_list(&list);
    assert(result != -1);
    assert(list == NULL);
}

int main(void)
{
    test_create_list();
    test_common_add_new_connection();
    test_common_add_new_secure_connection();
    test_common_get_client_info();
    test_common_remove_connection();
    return 0;
}
