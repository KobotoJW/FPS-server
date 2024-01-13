#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

const int PORT = 12345;
const int MAX_CLIENTS = 100;

struct Player {
    int id;
    float x;
    float y;
};

struct Bullet {
    int ownerID;
    float x;
    float y;
    float vx;
    float vy;
};

std::vector<Bullet> bullets;

int main() {
    int serverSocket;
    struct sockaddr_in serverAddr;
    std::vector<Player> players;
    std::vector<Bullet> bullets;
    fd_set readfds;
    int maxSocket = -1;

    // Create socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int enable = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }
    

    // Setup server address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Bind failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, MAX_CLIENTS) == -1) {
        perror("Listen failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is listening on port " << PORT << std::endl;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);
        maxSocket = serverSocket;

        // Add existing client sockets to the set
        for (const Player& player : players) {
            int clientSocket = player.id;
            FD_SET(clientSocket, &readfds);
            if (clientSocket > maxSocket) {
                maxSocket = clientSocket;
            }
        }

        // Use select to wait for activity on any socket
        int activity = select(maxSocket + 1, &readfds, nullptr, nullptr, nullptr);

        if ((activity < 0) && (errno != EINTR)) {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        // Handle new connection
        if (FD_ISSET(serverSocket, &readfds)) {
            int clientSocket;
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);

            if ((clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen)) == -1) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            std::cout << "New connection from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;

            // Send the assigned ID to the client
            send(clientSocket, &clientSocket, sizeof(clientSocket), 0);

            Player newPlayer;
            newPlayer.id = clientSocket;
            newPlayer.x = 0.f;
            newPlayer.y = 0.f;
            players.push_back(newPlayer);
        }

        // Handle client activity
        for (auto it = players.begin(); it != players.end(); ++it) {
            int clientSocket = it->id;

            if (FD_ISSET(clientSocket, &readfds)) {
                Player updatedPlayer;
                ssize_t bytesRead = recv(clientSocket, &updatedPlayer, sizeof(updatedPlayer), 0);

                if (bytesRead <= 0) {
                    std::cout << "Client disconnected: " << clientSocket << std::endl;

                    // Remove the disconnected player
                    close(clientSocket);
                    it = players.erase(it);
                } else {
                    // Update the player's position
                    it->x = updatedPlayer.x;
                    it->y = updatedPlayer.y;
                }

                // Broadcast the updated player list to all clients
                for (const Player& player : players) {
                    send(player.id, &player, sizeof(player), 0);
                }

                if (bytesRead <= 0 && it == players.end()) {
                    break;  // Exit loop if player was erased
                }

                // Handle shooting (For simplicity, this example doesn't handle bullet collisions)
                Bullet newBullet;
                bytesRead = recv(clientSocket, &newBullet, sizeof(newBullet), MSG_DONTWAIT);

                if (bytesRead > 0) {
                    bullets.push_back(newBullet);

                    // Broadcast the new bullet to all clients
                    for (const Player& player : players) {
                        send(player.id, &newBullet, sizeof(newBullet), 0);
                    }
                }

                // Broadcast existing bullets to the current client
                for (const Bullet& bullet : bullets) {
                    send(clientSocket, &bullet, sizeof(bullet), 0);
                }
            }
        }

        // Update the positions of the bullets
        for (Bullet& bullet : bullets) {
            bullet.x += bullet.vx;
            bullet.y += bullet.vy;
        }

        // Handle data from clients
        for (int i = 0; i <= maxSocket; i++) {
            if (FD_ISSET(i, &readfds)) {
                // Receive data from the client
                Bullet newBullet;
                if (recv(i, &newBullet, sizeof(newBullet), 0) > 0) {
                    // Add the new bullet to the list
                    bullets.push_back(newBullet);
                }
            }
        }

        // Send the updated bullet list to all clients
        for (int i = 0; i <= maxSocket; i++) {
            if (FD_ISSET(i, &readfds)) {
                if (!bullets.empty()) {
                    if (send(i, &bullets[0], bullets.size() * sizeof(Bullet), 0) == -1) {
                        perror("Send failed");
                    }
                }
            }
        }

        usleep(10000);  // Sleep for 10 milliseconds (adjust as needed)
    }

    close(serverSocket);

    return 0;
}
