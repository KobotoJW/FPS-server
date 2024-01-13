#include "Server.h"
#include <iostream>
#include <string.h>
#include <algorithm>
#include <unistd.h>
#include <mutex>

std::mutex clientsMutex;

Server::Server(int maxPlayersPerRoom) : roomCount(0), maxPlayersPerRoom(maxPlayersPerRoom), nextClientId(1) {
    commandThread = std::thread(&Server::handleCommands, this);
}

Server::~Server() {
    if (commandThread.joinable()) {
        commandThread.join();
    }
}

Room* Server::findAvailableRoom() {
    for (Room& room : rooms) {
        if (!room.isFull()) {
            return &room;
        }
    }
    // If no available room is found, create a new one
    std::cout << "Creating new room" << std::endl;
    rooms.push_back(Room("Room" + std::to_string(roomCount++), maxPlayersPerRoom));
    return &rooms.back();
}

void Server::handleNewConnection(int newSocket, struct sockaddr_in clientAddress) {
    Room* room = findAvailableRoom();
    std::string name = "Client" + std::to_string(nextClientId);
    Client* newClient = new Client(clientAddress, name, nextClientId);
    nextClientId++;
    room->addClient(newClient);
    sendWelcomePacket(newClient, newSocket, newClient->getId());
}

void Server::handleExistingConnections(fd_set* masterSet, int serverSocket) {
    for (Room& room : rooms) {
        auto& clients = room.getClients();
        for (auto it = clients.begin(); it != clients.end();) {
            Client* client = *it;
            if (FD_ISSET(client->getSocket(), masterSet)) {
                int senderId = -1;
                Packet packet = client->receivePacket(serverSocket);
                // Handle the received packet
                switch (packet.type)
                {
                case 0: // Disconnect packet
                    senderId = packet.id;
                    if (senderId == client->getId() && senderId > 0) {
                        // Remove the client from the room
                        room.removeClient(client);
                        // Remove the client from the master set
                        FD_CLR(client->getSocket(), masterSet);
                        close(client->getSocket());
                        // Delete the client
                        delete client;
                        // Remove the client from the clients vector
                        clientsMutex.lock();
                        it = clients.erase(it);
                        clientsMutex.unlock();
                    } else {
                        ++it;
                    }
                
                default:
                    ++it;
                    break;
                }
            }
        }
    }
}

void Server::sendWelcomePacket(Client* client, int clientSocket, int clientId) {
    Packet welcomePacket;
    welcomePacket.type = 1; // Welcome packet type
    strcpy(welcomePacket.data, "Welcome to the server!");
    welcomePacket.id = clientId;
    client->sendPacket(welcomePacket, clientSocket);
}

void Server::handleCommands() {
    std::string command;
    while (true) {
        std::getline(std::cin, command);
        if (command == "/status") {
            // Print rooms list and client IDs
            for (auto& room : rooms) {
                std::cout << "Room: " << room.getName() << std::endl;
                for (const auto& client : room.getClients()) {
                    std::cout << "  Client ID: " << client->getId() << std::endl;
                }
            }
        }
    }
}