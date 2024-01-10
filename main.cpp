#include <iostream>
#include <cstring>
#include <thread>
#include <enet/enet.h>

void sendPacket(ENetPeer * peer, char * message)
{
    ENetPacket * packet = enet_packet_create(message, strlen(message) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet); //(where, channel, packet)
    enet_host_flush(peer->host);
}

int main(int argc, char ** argv)
{
    if(enet_initialize() != 0)
    {
        std::cerr << "An error occurred while initializing ENet." << std::endl;
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetAddress address;
    ENetEvent event;

    address.host = ENET_HOST_ANY;
    address.port = 2115;

    ENetHost * server;

    server = enet_host_create(&address, 32, 1, 0, 0);

    if (server == NULL)
    {
        std::cerr << "An error occurred while trying to create an ENet server host." << std::endl;
        return EXIT_FAILURE;
    }

    //game loop
    char message[256] = "Welcome to the server!";

    while (true)
    {
        while (enet_host_service(server, &event, 1000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "A new client connected from " << event.peer->address.host << ":" << event.peer->address.port << "." << std::endl;
                sendPacket(event.peer, message);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                std::cout << "A packet of length " << event.packet->dataLength << " containing message: " << event.packet->data << " was received from " << event.peer->address.host << ":" << event.peer->address.port << " on channel " << event.channelID << "." << std::endl;
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << event.peer->address.host << ":" << event.peer->address.port << " disconnected." << std::endl;
                event.peer->data = NULL;
                break;
            }
        }
    }
    //game loop end
    
    enet_host_destroy(server);
    return EXIT_SUCCESS;
}