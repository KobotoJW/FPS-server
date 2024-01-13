#pragma once
#include <vector>
#include "Client.h"

class Room {
public:
    Room(const std::string& name, int maxPlayers);
    void addClient(Client* client);
    void broadcastPacket(const Packet& packet);
    bool isFull() const;
    std::vector<Client*>& getClients();
    int getClientCount() const;
    std::string getName() const;
    void removeClient(Client* client);
private:
    std::vector<Client*> clients;
    std::string name;
    int maxPlayers;
};