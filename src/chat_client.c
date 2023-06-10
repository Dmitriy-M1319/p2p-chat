#include "udp_client_connection_query.h"
#include "connection_list.h"
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

// Список всех подключений для клиентам в локальной сети
client_connection *connections = NULL;


void query_invitation_in_network(const char *client_name)
{
    int query_socket;
    struct sockaddr_in info;

    // Создаем сокет для широковещательного запроса в сеть
    if ((query_socket = create_udb_broadcast_socket()) < 0) {
        exit(1);
    }
    // Отправляем широковещательный запрос
    if (send_connection_query(query_socket, client_name) == -1) {
        exit(2);
    }

    // Тут надо каким-то образом дождаться 6 сек, если за это время

    struct query_datagramm data;
    struct sockaddr addr;
    socklen_t addr_len;
    int new_cl_socket;
    // Ждем получения ответа от других клиентов чата (походу это надо в отдельный поток вынести
    // как разделенное управление одним сокетом
    if (recvfrom(query_socket, &data, sizeof(data), 0, &addr, &addr_len) == -1) {
        fprintf(stderr, "failed to get connection_info from other client");
        exit(3);
    }
    
    connections = create_client_list();
    // Создаем новое подключение с клиентом (надо в отдельном потоке делать)
    if(create_client_connection(&data, connections) == -1) {
        fprintf(stderr, "Failed to create new connection to client\n");
        exit(3);
    }
}




int main(int argc, char *argv[])
{
    printf("Hello, I am future chat client...\n");
    printf("Send a msg on local broadcast..\n");
    return 0;
}
