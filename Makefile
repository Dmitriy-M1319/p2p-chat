CC = gcc
CFLAGS = -g -Wall -Wextra -pedantic -pthread -lssl -lcrypto

p2p-chat: connection_list.o udp_client_connection_query.o messenger.o ssl_utils.o src/chat_client.c
	$(CC) $(CFLAGS) connection_list.o udp_client_connection_query.o messenger.o ssl_utils.o src/chat_client.c -o bin/chat_client

connection_list.o: src/connection_list.c
	$(CC) $(CFLAGS) -c src/connection_list.c

udp_client_connection_query.o: src/udp_client_connection_query.c
	$(CC) $(CFLAGS) -c src/udp_client_connection_query.c

messenger.o: src/messenger.c
	$(CC) $(CFLAGS) -c src/messenger.c

ssl_utils.o: src/ssl_utils.c
	$(CC) $(CFLAGS) -c src/ssl_utils.c

clean:
	rm *.o
	rm bin/chat_client
