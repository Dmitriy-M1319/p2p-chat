#include "../../src/udp_client_connection_query.h"
#include <stdio.h>
#include <netinet/in.h>
#include <assert.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>


void test_create_udp_broadcast_socket()
{
    int result;
    result = create_udb_broadcast_socket();
    assert(result != -1);
    assert(close(result) != -1);
}

void test_create_simple_tcp_socket()
{
    int result;
    result = create_tcp_client_socket();
    assert(result != -1);
    assert(close(result) != -1);
}

void test_create_simple_udp_socket()
{
    int result;
    result = create_simple_udp_socket();
    assert(result != -1);
    assert(close(result) != -1);
}

void test_get_local_addr()
{
    struct sockaddr_in addr;
    socklen_t addr_length = sizeof(addr);

    int result = get_local_addr(&addr, &addr_length);
    assert(result == 0);

    assert(addr.sin_family == AF_INET);
    assert(addr.sin_addr.s_addr != 0);
}

int main(void)
{
    test_create_udp_broadcast_socket();
    test_create_simple_tcp_socket();
    test_create_simple_udp_socket();
    test_get_local_addr();
    return 0;
}
