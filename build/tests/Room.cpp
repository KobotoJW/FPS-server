#include "Room.h"
#include "Client.h"
#include <iostream>

Room::Room(const std::string& name, int maxPlayers) : name(name), maxPlayers(maxPlayers) {}

void Room::addClient(Client* client) {
    clients.push_back(client);
}

void Room::broadcastPacket(const Packet& packet) {
    for (Client* client : clients) {
        client->sendPacket(packet, client->getSocket());
    }
}

bool Room::isFull() const {
    std::cout << maxPlayers << " Checking if room is full " << (clients.size() >= maxPlayers) << std::endl;
    return clients.size() >= maxPlayers;
}

int Room::getClientCount() const {
    return clients.size();
}

std::string Room::getName() const {
    return name;
}

std::vector<Client*>& Room::getClients() {
    return clients;
}

void Room::removeClient(Client* client) {
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (*it == client) {
            clients.erase(it);
            break;
        }
    }
}