#pragma once

enum MessageType: uint8_t{ //enum means fixed values 
    MSG_PING = 1,
    MSG_ACK = 2,
    MSG_DATA = 3
};