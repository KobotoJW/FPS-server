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
//const std::string CAP_PRE = "5t4rt";
const std::string CAP_POST = "5t0p";


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
            removeClientSocket(clientSocket);
        } else if (bytesRead == 0) {
            // Connection closed by client
            std::cout << "Client disconnected" << std::endl;
            close(clientSocket);
            removeClientSocket(clientSocket);
        } else {
            // Handle the received data
            // Decapsulate the data and add them to list
            std::cout << "Buffer: " << buffer << std::endl;

            std::vector<std::string> dataV;
            std::string dataString = buffer;
            size_t start = 0;
            size_t end = 0;

            std::cout << "Received data from client pre decap: " << dataString << std::endl;

            while ((end = dataString.find(CAP_POST, start)) != std::string::npos) {
                std::string data = dataString.substr(start, end - start);
                dataString = dataString.substr(end + CAP_POST.size());
                start = end + CAP_POST.size();
                start = 0;
                dataV.push_back(data);
            }

            for (std::string d : dataV) {
                std::cout << "Received data from client cap: " << d << std::endl;
                handleReceivedData(clientSocket, d.c_str(), d.size());
            }
        }
    }

    void handleReceivedData(int clientSocket, const char* data, ssize_t dataSize) {
        // Handle the received data from the client

        if (dataSize == 4 && strncmp(data, "ping", 4) == 0) {
            //std::cout << "Received ping from client" << std::endl;
            returnPong(clientSocket);
            return;
        }

        //std::cout << "Received data from client: " << data << std::endl;
        nlohmann::json dataJson = nlohmann::json::parse(data);
        //std::cout << "Received data from client (JSON): " << dataJson << std::endl;

        if (dataJson.contains("type") && dataJson["type"] == "bullet") {
            // Send the bullet to all clients
            std::string dataJsonCap = dataJson.dump() + CAP_POST;
            for (int client : clientSockets) {
                send(client, dataJsonCap.c_str(), dataJsonCap.size(), 0);
            }
        } else if (dataJson.contains("type") && dataJson["type"] == "player"){
            // Send the player to all clients
            std::string dataJsonCap = dataJson.dump() + CAP_POST;
            for (int client : clientSockets) {
                if (client != clientSocket){
                    send(client, dataJsonCap.c_str(), dataJsonCap.size(), 0);
                }            
            }
        }
        
    }

    void returnPong(int clientSocket) {
        std::string pongMsg = "pong" + CAP_POST;
        send(clientSocket, pongMsg.c_str(), pongMsg.size(), 0);
        //std::cout << "Returned pong to client" << std::endl;
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
