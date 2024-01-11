#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

const char* SERVER_IP = "127.0.0.1";
const int PORT = 12345;

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

    char buffer[1024];
    while (true) {
        // Get user input or implement your own game logic
        std::cout << "Enter a message (or 'exit' to quit): ";
        std::cin.getline(buffer, sizeof(buffer));

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        // Send user input to the server
        send(clientSocket, buffer, strlen(buffer), 0);

        // Receive and display the server's response
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            std::cerr << "Server disconnected" << std::endl;
            break;
        }

        buffer[bytesRead] = '\0';
        std::cout << "Server response: " << buffer << std::endl;
    }

    // Close the client socket
    close(clientSocket);

    return 0;
}
