#include "udp_client_connection_query.h"
#include "connection_list.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Список всех подключений для клиентам в локальной сети
struct client_connection_node *connections = NULL;

void query_invitation_in_network()
{
    int query_socket;
    struct sockaddr_in info;

    query_socket = create_udb_broadcast_socket();
        // Подумать над обработкой разных ошибок
    if (send_connection_query(query_socket) == -1) {
        exit(1);
    }

    // дальше пока не понимать...

}



int main(int argc, char *argv[])
{
    printf("Hello, I am future chat client...\n");
    printf("Send a msg on local broadcast..\n");
    return 0;
}
