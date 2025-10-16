#ifndef STRUCTS_H
#define STRUCTS_H

#include <iostream>

typedef struct WRPLRawPacket {
    int currentTime;
    std::byte packetType;
    std::byte packetPayload[];
};

#endif // STRUCTS_H
