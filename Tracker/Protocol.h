#pragma once

enum MessageType: uint8_t{ //enum means fixed values 
    MSG_REGISTER    = 1,
    MSG_HEARTBEAT   = 2,
    MSG_GET_PEERS    = 3,
    MSG_PEER_LIST   = 4,
    MSG_ACK         = 5
};