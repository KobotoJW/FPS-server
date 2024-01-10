#include <iostream>
#include <cstring>
#include <enet/enet.h>
#include <thread>

char* getUsername()
{
    char* username = new char[32];
    std::cout << "Enter username: ";
    fgets(username, 32, stdin);
    return username;
}

void sendPacket(ENetPeer * peer, char * message)
{
    ENetPacket * packet = enet_packet_create(message, strlen(message)+1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet); //(where, channel, packet)
    enet_host_flush(peer->host);
}

void * RecieveLoop(ENetHost * client)
{
    while (true)
    {
        ENetEvent event;
        while (enet_host_service(client, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                std::cout << "A packet of length " << event.packet->dataLength << " containing message: " << event.packet->data << " was received from " << event.peer->address.host << ":" << event.peer->address.port << " on channel " << event.channelID << "." << std::endl;
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "Disconnection succeeded." << std::endl;
                break;
            }
        }
    }
    
}

void *  sendLoop(ENetPeer * peer)
{
    char message[256];
    while (true)
    {
        std::cout << "Enter message: ";
        fgets(message, sizeof(message), stdin);
        sendPacket(peer, message);
    }
}

int main(int argc, char ** argv)
{
    char * username = getUsername();

    if (enet_initialize() != 0)
    {
        std::cerr << "An error occurred while initializing ENet." << std::endl;
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetHost * client;
    client = enet_host_create(NULL, 1, 1, 0, 0);

    if (client == NULL)
    {
        std::cerr << "An error occurred while trying to create an ENet client host." << std::endl;
        return EXIT_FAILURE;
    }

    ENetAddress address;
    ENetEvent event;
    ENetPeer * peer;

    enet_address_set_host(&address, "localhost");
    address.port = 2115;

    peer = enet_host_connect(client, &address, 1, 0);
    if (peer == NULL)
    {
        std::cerr << "No available peers for initiating an ENet connection." << std::endl;
        return EXIT_FAILURE;
    }

    if (enet_host_service(client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
    {
        std::cout << "Connection to localhost:2115 succeeded." << std::endl;
    }
    else
    {
        enet_peer_reset(peer);
        std::cout << "Connection to localhost:2115 failed." << std::endl;
        return EXIT_SUCCESS; //here can return to main menu, retry, etc.
    }

    char message[256];
    //Game loop
    std::thread messageLoop(RecieveLoop, client);
    
    sendLoop(peer);

    //Game loop end
    

    //enet_peer_disconnect(peer, 0);

    return EXIT_SUCCESS;
}