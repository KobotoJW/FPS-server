#pragma once
#include <string>

/*
    Packet types:
    0 - Disconnect packet,
    1 - Welcome packet,
*/

struct Packet {
    int type;
    int id;
    char data[1024];
};