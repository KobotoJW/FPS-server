#include "Client.h"
#include "Packet.h"
#include <sys/socket.h>

Client::Client(struct sockaddr_in clientAddress, const std::string& name, int id) : clientAddress(clientAddress), name(name), id(id), disconnected(false) {}

void Client::sendPacket(const Packet& packet, int socket) {
    sendto(socket, &packet, sizeof(packet), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
}

Packet Client::receivePacket(int serverSocket) {
    Packet packet;
    socklen_t len = sizeof(clientAddress);
    recvfrom(serverSocket, &packet, sizeof(packet), 0, (struct sockaddr*)&clientAddress, &len);
    return packet;
}

int Client::getId() const {
    return id;
}

int Client::getSocket() const {
    return clientAddress.sin_port;
}

Room* Client::getRoom() {
    return room;
}