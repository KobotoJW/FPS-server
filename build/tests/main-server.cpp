#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include "Server.h"
#include "Client.h"
#include "Packet.h"
#include "Room.h"

#define PORT 8080
#define MAX_PLAYERS_PER_ROOM 2

int main(int argc, char** argv) {
    Server server(MAX_PLAYERS_PER_ROOM);

    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("Failed to create socket");
        return 1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(listener, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Failed to bind socket");
        return 1;
    }

    if (listen(listener, 10) < 0) {
        perror("Failed to listen on socket");
        return 1;
    }

    std::cout << "Server is listening on port " << PORT << std::endl;

    while (true) {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLen = sizeof(clientAddress);

        int newSocket = accept(listener, (struct sockaddr*)&clientAddress, &clientAddressLen);
        if (newSocket < 0) {
            perror("Failed to accept new connection");
            continue;
        }

        server.handleNewConnection(newSocket, clientAddress);

        fd_set masterSet;
        FD_ZERO(&masterSet);
        FD_SET(listener, &masterSet);
        server.handleExistingConnections(&masterSet, listener);
    }

    close(listener);

    return 0;
}