#pragma once
#include <string>
#include "Packet.h"
#include "Room.h"
#include <netinet/in.h>

class Client {
public:
    Client(struct sockaddr_in clientAddress, const std::string& name, int id);
    void sendPacket(const Packet& packet, int serverSocket);
    Packet receivePacket(int serverSocket);
    int getId() const;
    int getSocket() const;
    Room* getRoom();
private:
    struct sockaddr_in clientAddress;
    std::string name;
    int id;
    bool disconnected;
    Room* room;
};