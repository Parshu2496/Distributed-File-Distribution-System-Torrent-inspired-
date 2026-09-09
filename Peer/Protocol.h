#pragma once
#include <cstdint>

enum MessageType : uint8_t {
    MSG_REGISTER          = 1,
    MSG_HEARTBEAT         = 2,
    MSG_GET_PEERS         = 3,
    MSG_PEER_LIST         = 4,
    MSG_ACK               = 5,
    MSG_PING              = 6,
    MSG_REQUEST_FILE_INFO = 7,  // downloader → seeder: ask for file metadata
    MSG_FILE_INFO         = 8,  // seeder → downloader: total chunks + file size
    MSG_REQUEST_CHUNK     = 9,  // downloader → seeder: ask for chunk N
    MSG_CHUNK_DATA        = 10, // seeder → downloader: raw chunk bytes
};

// Size of each chunk in bytes (512 KB)
static const uint32_t CHUNK_SIZE = 512 * 1024;