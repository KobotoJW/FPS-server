#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <algorithm>
#include <mutex>

int PORT;
int MAX_CLIENTS;

struct Room;

struct Client {
    int socket;
    std::string name;
    Room* room;
    bool disconnected = false;
};

struct Room {
    std::string name;
    std::vector<Client*> clients;

    Room() {}
    Room(const std::string& roomName) : name(roomName) {}

    void removeClient(Client* client) {
        clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());
    }

    void informClients() {
        for (auto& client : clients) {
            std::string message = "In this room: ";
            for (auto& otherClient : clients) {
                if (otherClient != client) {
                    message += otherClient->name + ", ";
                }
            }
            message = message.substr(0, message.size() - 2); // Remove trailing comma
            send(client->socket, message.c_str(), message.size(), 0);
        }
    }
};

std::vector<Room> rooms;
std::mutex roomsMutex;

// Forward declarations
void printStatus();
void cleanupEmptyRooms();
void handleClientDisconnect(Client* client);
void handleClient(Client* client, fd_set* masterSet);
void startServer(int PORT, int MAX_CLIENTS);

// Function to print the server status
void printStatus() {
    std::lock_guard<std::mutex> lock(roomsMutex);
    std::cout << "Rooms: " << rooms.size() << std::endl;
    for (const auto& room : rooms) {
        std::cout << "-Room " << room.name << ": " << room.clients.size() << " clients" << std::endl;
        for (const auto& client : room.clients) {
            std::cout << "--Client: " << client->name << std::endl;
        }
    }
}

// Function to clean up empty rooms
void cleanupEmptyRooms() {
    std::thread([]() {
        std::lock_guard<std::mutex> lock(roomsMutex);
        for (auto it = rooms.begin(); it != rooms.end();) {
            if (it->clients.empty() && it->name != "Room 1") {
                std::cout << "Room " << it->name << " is now empty, removing it" << std::endl;
                it = rooms.erase(it);
            } else {
                ++it;
            }
        }
    }).detach();
}

// Function to handle client disconnection
void handleClientDisconnect(Client* client) {
    if (!client || client->disconnected) {
        return;
    }

    // Make a copy of the room pointer
    Room* room = client->room;

    // Remove the client from the room
    if (room) {
        room->removeClient(client);
    }

    // Close the client's socket
    close(client->socket);
    client->disconnected = true;

    // Cleanup empty rooms
    cleanupEmptyRooms();
}

// Function to handle client communication
void handleClient(Client* client, fd_set* masterSet) {
    if (!client || client->disconnected) {
        return;
    }

    char buffer[1024];
    int bytesRead;

    if ((bytesRead = recv(client->socket, buffer, sizeof(buffer), 0)) > 0) {
        // Process received data
        buffer[bytesRead] = '\0';
        if (client && client->room) {
            std::cout << "Received from " << client->name << " in room " << client->room->name << ": " << buffer << std::endl;
        } else {
            std::cout << "Received: " << buffer << std::endl;
        }
        // Implement your game logic here

        // Example: Send a response back to the client
        send(client->socket, "Message received", strlen("Message received"), 0);
    } else {
        // Client disconnected
        std::cout << "Client disconnected" << std::endl;
        handleClientDisconnect(client);
        FD_CLR(client->socket, masterSet);
    }
}

int roomCount = 1;

// Function to start the server
void startServer(int PORT, int MAX_CLIENTS) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress;

    if (serverSocket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "Error setting socket options" << std::endl;
        exit(EXIT_FAILURE);
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    listen(serverSocket, MAX_CLIENTS);

    fd_set masterSet;
    FD_ZERO(&masterSet);
    FD_SET(serverSocket, &masterSet);
    int maxSocket = serverSocket;

    while (true) {
        fd_set copySet = masterSet;

        if (select(maxSocket + 1, &copySet, nullptr, nullptr, nullptr) == -1) {
            perror("select");
            exit(4);
        }

        for (int i = 0; i <= maxSocket; i++) {
            if (FD_ISSET(i, &copySet)) {
                if (i == serverSocket) {
                    // New connection
                    sockaddr_in clientAddress;
                    socklen_t clientSize = sizeof(clientAddress);
                    int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientSize);

                    if (clientSocket == -1) {
                        std::cerr << "Error accepting connection" << std::endl;
                        continue;
                    }

                    FD_SET(clientSocket, &masterSet);
                    if (clientSocket > maxSocket) {
                        maxSocket = clientSocket;
                    }

                    // Add the client to a room
                    if (rooms.empty()) {
                        rooms.emplace_back("Room1");
                    }

                    Room* room = nullptr;
                    for (auto& r : rooms) {
                        if (r.clients.size() < MAX_CLIENTS) {
                            room = &r;
                            break;
                        }
                    }

                    if (!room) {
                        // All rooms are full, create a new one
                        roomCount++;
                        rooms.push_back(Room("Room" + std::to_string(roomCount)));
                        room = &rooms.back();
                    }

                    room->clients.emplace_back(new Client{clientSocket, "Player" + std::to_string(room->clients.size() + 1)});
                    std::cout << "Client connected: " << room->clients.back()->name << " to room: " << room->name << std::endl;
                } else {
                    std::vector<Room*> roomsToClean;
                    {
                        std::lock_guard<std::mutex> lock(roomsMutex);
                        for (auto& room : rooms) {
                            for (auto& client : room.clients) {
                                if (client && client->socket == i) {
                                    handleClient(client, &masterSet);
                                    if (client->disconnected) {
                                        roomsToClean.push_back(&room);
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    for (auto& room : roomsToClean) {
                        room->clients.erase(std::remove_if(room->clients.begin(), room->clients.end(),
                            [](const Client* client) { return client->disconnected; }), room->clients.end());
                    }
                }
            }
        }
    }
}

void broadcastToRoom(Room* room, const std::string& message) {
    for (auto& client : room->clients) {
        send(client->socket, message.c_str(), message.size(), 0);
    }
}

void startCommandThread() {
    std::thread commandThread([]() {
        std::string input;
        while (true) {
            std::getline(std::cin, input);
            if (input == "status") {
                printStatus();
            }
            
            if (input == "/say"){
                std::string message;
                std::cout << "Enter message: ";
                std::getline(std::cin, message);
                std::cout << "Enter room: ";
                std::getline(std::cin, input);
                std::lock_guard<std::mutex> lock(roomsMutex);
                for (auto& room : rooms) {
                    if (room.name == input) {
                        broadcastToRoom(&room, message);
                        break;
                    }
                }
            }
        }
    });
    commandThread.detach();
}

// Main function
int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <max_clients>" << std::endl;
        return 1;
    }
    PORT = std::stoi(argv[1]);
    MAX_CLIENTS = std::stoi(argv[2]);

    startCommandThread();
    startServer(PORT, MAX_CLIENTS);
    return 0;
}
