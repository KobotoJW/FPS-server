#pragma once
#include <vector>
#include "Room.h"
#include <mutex>
#include <thread>

class Server {
public:
    Server(int maxPlayersPerRoom);
    ~Server(); // destructor to join the thread
    void handleNewConnection(int newSocket, struct sockaddr_in clientAddress);
    void handleExistingConnections(fd_set* masterSet, int serverSocket);

private:
    std::vector<Room> rooms;
    int roomCount;
    int maxPlayersPerRoom;
    int nextClientId;
    std::mutex roomsMutex;
    std::thread commandThread;

    Room* findAvailableRoom();
    void sendWelcomePacket(Client* client, int serverSocket, int clientId);
    void handleCommands();
};