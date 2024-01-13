#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <csignal>

#include "Packet.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

int clientSocket;
int myID;

void handleSignal(int signal) {
    if (signal == SIGINT) {
        std::cout << "Ctrl+C pressed. Closing connection..." << std::endl;

        Packet packet;
        packet.type = 0; // Disconnect
        if (myID > 0){
            packet.id = myID;
        }

        if (send(clientSocket, &packet, sizeof(packet), 0) < 0) {
            perror("Failed to send disconnect packet");
            close(clientSocket);
            exit(0);
        }

        close(clientSocket);
        exit(0);
    }
}

int main(int argc, char** argv) {
    int myID = 0;
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("Failed to create socket");
        return 1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serverAddress.sin_addr) <= 0) {
        perror("Failed to set server address");
        return 1;
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Failed to connect to server");
        return 1;
    }

    while (true)
    {
        Packet packet;
        int bytesReceived = read(clientSocket, &packet, sizeof(packet));
        if (bytesReceived < 0) {
            perror("Failed to read from socket");
            return 1;
        }
        else if (bytesReceived == 0) {
            std::cout << "Connection closed by server" << std::endl;
            return 0;
        }
        else {
            switch (packet.type)
            {
            case 1 /* Welcome */:
                std::cout << "Received packet from server: " << std::endl;
                std::cout << "Type: " << packet.type << std::endl;
                myID = packet.id;
                std::cout << "My new ID: " << myID << std::endl;
                std::cout << "Data: " << packet.data << std::endl;

                break;
            
            default:
                break;
            }
            
        }
        signal(SIGINT, handleSignal);
    }

    close(clientSocket);

    return 0;
}