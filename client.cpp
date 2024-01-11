#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>

const char* SERVER_IP = "127.0.0.1";
const int PORT = 12345;

void sendToServer(int clientSocket, const char* message) {
    send(clientSocket, message, strlen(message), 0);
}

void startCommandThread(int clientSocket) {
    std::thread commandThread([clientSocket]() {
        std::string input;
        while (true) {
            std::getline(std::cin, input);
            if (input == "/say"){
                std::string message;
                std::cout << "Enter message: ";
                std::getline(std::cin, message);
                sendToServer(clientSocket, message.c_str());
            }
        }
    });
    commandThread.detach();
}

int main() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error connecting to server" << std::endl;
        close(clientSocket);
        return 1;
    }

    std::cout << "Connected to server" << std::endl;

    startCommandThread(clientSocket);

    while (true) {
        char buffer[1024];
        // Receive and display the server's response
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            std::cerr << "Server disconnected" << std::endl;
            break;
        }

        buffer[bytesRead] = '\0';
        std::cout << "Server: " << buffer << std::endl;
    }

    // Close the client socket
    close(clientSocket);

    return 0;
}
