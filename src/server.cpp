#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <arpa/inet.h>

constexpr int MAX_EVENTS = 10;
constexpr int PORT = 5000;

class Server {
public:
    Server() : serverSocket(-1), epollFd(-1) {}

    void run() {
        if (initializeServer() && initializeEpoll()) {
            std::cout << "Server is running on port " << PORT << std::endl;
            epollLoop();
        }
    }

private:
    int serverSocket;
    int epollFd;
    std::vector<int> clientSockets;

    bool initializeServer() {
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
            perror("Error creating server socket");
            return false;
        }

        if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
            perror("Error binding server socket");
            close(serverSocket);
            return false;
        }

        if (listen(serverSocket, SOMAXCONN) == -1) {
            perror("Error listening on server socket");
            close(serverSocket);
            return false;
        }

        return true;
    }

    bool initializeEpoll() {
        epollFd = epoll_create1(0);
        if (epollFd == -1) {
            perror("Error creating epoll");
            close(serverSocket);
            return false;
        }

        epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = serverSocket;

        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, &event) == -1) {
            perror("Error adding server socket to epoll");
            close(epollFd);
            close(serverSocket);
            return false;
        }

        return true;
    }

    void acceptConnection() {
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);

        int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
        if (clientSocket == -1) {
            perror("Error accepting connection");
            return;
        }

        std::cout << "New connection from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;

        // Add the new client socket to the epoll set
        epoll_event event;
        event.events = EPOLLIN | EPOLLET;  // Edge-triggered
        event.data.fd = clientSocket;

        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1) {
            perror("Error adding client socket to epoll");
            close(clientSocket);
            return;
        }

        clientSockets.push_back(clientSocket);
        sendMapToClient(clientSocket);
    }

    void sendMapToClient(int clientSocket) {
        //send map as json to client
        nlohmann::json map;
        map = {
            {"dimensions", {
                {"width", 800},
                {"height", 600}
            }},

            {"floor", {
                {"position", {
                    {"x", 0},
                    {"y", 0}
                }},
                {"dimensions", {
                    {"width", 800},
                    {"height", 600}
                }},
                {"color", {
                    {"r", 100},
                    {"g", 100},
                    {"b", 100}
                }}
            }},

            {"walls", {
                {
                    {"position", {
                        {"x", 0},
                        {"y", 0}
                    }},
                    {"dimensions", {
                        {"width", 800},
                        {"height", 50}
                    }},
                    {"color", {
                        {"r", 50},
                        {"g", 50},
                        {"b", 50}
                    }}
                },
                {
                    {"position", {
                        {"x", 0},
                        {"y", 0}
                    }},
                    {"dimensions", {
                        {"width", 50},
                        {"height", 600}
                    }},
                    {"color", {
                        {"r", 50},
                        {"g", 50},
                        {"b", 50}
                    }}
                },
                {
                    {"position", {
                        {"x", 0},
                        {"y", 550}
                    }},
                    {"dimensions", {
                        {"width", 800},
                        {"height", 50}
                    }},
                    {"color", {
                        {"r", 50},
                        {"g", 50},
                        {"b", 50}
                    }}
                },
                {
                    {"position", {
                        {"x", 750},
                        {"y", 0}
                    }},
                    {"dimensions", {
                        {"width", 50},
                        {"height", 600}
                    }},
                    {"color", {
                        {"r", 50},
                        {"g", 50},
                        {"b", 50}
                    }}
                },
                {
                    {"position", {
                        {"x", 200},
                        {"y", 200}
                    }},
                    {"dimensions", {
                        {"width", 400},
                        {"height", 50}
                    }},
                    {"color", {
                        {"r", 50},
                        {"g", 50},
                        {"b", 50}
                    }}
                }
            }}
        };

        std::string mapString = map.dump();
        send(clientSocket, mapString.c_str(), mapString.size(), 0);

    }

    void handleClientData(int clientSocket) {
        char buffer[1024] = {0};
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesRead == -1) {
            perror("Error receiving data from client");
        } else if (bytesRead == 0) {
            // Connection closed by client
            std::cout << "Client disconnected" << std::endl;
            close(clientSocket);
            removeClientSocket(clientSocket);
        } else {
            // Handle the received data TODO
            handleReceivedData(clientSocket, buffer, bytesRead);
        }
    }

    void handleReceivedData(int clientSocket, const char* data, ssize_t dataSize) {
        // Handle the received data from the client
        // For now, just print the received message
        if (dataSize == 4 && strncmp(data, "ping", 4) == 0) {
            returnPong(clientSocket);
            return;
        }
        nlohmann::json dataJson = nlohmann::json::parse(data);
        std::cout << "Received data from client: " << dataJson << std::endl;
    }

    void returnPong(int clientSocket) {
        send(clientSocket, "pong", 4, 0);
    }

    void removeClientSocket(int clientSocket) {
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientSocket, nullptr);
        close(clientSocket);

        auto it = std::remove(clientSockets.begin(), clientSockets.end(), clientSocket);
        clientSockets.erase(it, clientSockets.end());
    }

    void epollLoop() {
        epoll_event events[MAX_EVENTS];

        while (true) {
            int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, -1);

            for (int i = 0; i < numEvents; ++i) {
                if (events[i].data.fd == serverSocket) {
                    // New connection
                    acceptConnection();
                } else {
                    // Data received from a client
                    handleClientData(events[i].data.fd);
                    
                }
            }
        }
    }
};

int main() {
    Server server;
    server.run();

    return 0;
}
