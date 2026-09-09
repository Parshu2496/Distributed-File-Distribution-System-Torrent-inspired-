#include "Peer.h"
#include "ConnectionHandler.h"
#include "Protocol.h"
#include "FileManager.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>   // stat() — check if directory exists
#include <iostream>
#include <cstdint>
#include <cstring>
#include <thread>
#include <vector>
#include <stdexcept>

// ---------------------------------------------
//  Constructor
// ---------------------------------------------
Peer::Peer(int port, const std::string& filesDir)
    : port(port), filesDir(filesDir) {}

// ---------------------------------------------
//  Public: blocking start (used in old Peer::start())
// ---------------------------------------------
void Peer::start() {
    std::cout << "[Peer:" << port << "] Starting\n";
    std::thread t(&Peer::startListener, this);
    t.join(); // startListener runs forever, so this blocks
}

// ---------------------------------------------
//  Utility: frame and send one message
//  Wire format: [4-byte length][1-byte type][payload...]
// ---------------------------------------------
void Peer::sendMessage(int sock, uint8_t type, const std::vector<char>& payload) {
    uint32_t bodyLen = htonl(static_cast<uint32_t>(1 + payload.size()));
    send(sock, &bodyLen, sizeof(bodyLen), 0);
    send(sock, &type,    sizeof(type),    0);
    if (!payload.empty())
        send(sock, payload.data(), payload.size(), 0);
}

// ---------------------------------------------
//  Accept loop
// ---------------------------------------------
void Peer::startListener() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) { std::cerr << "[Peer:" << port << "] socket() failed\n"; return; }

    // Allow quick rebind after restart
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(serverSocket, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[Peer:" << port << "] bind() failed\n";
        close(serverSocket); return;
    }
    if (listen(serverSocket, 10) < 0) {
        std::cerr << "[Peer:" << port << "] listen() failed\n";
        close(serverSocket); return;
    }
    // Print CWD + resolved files path so the user knows exactly where to put files
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd))) {
        std::cout << "[Peer:" << port << "] CWD         : " << cwd << "\n";
        std::cout << "[Peer:" << port << "] Files dir   : " << cwd << "/" << filesDir << "\n";
    }

    // Warn if the files directory doesn't exist
    struct stat st{};
    if (stat(filesDir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        std::cerr << "[Peer:" << port << "] ⚠️  WARNING: files dir '" << filesDir
                  << "' does not exist in CWD. Create it and add files before requesting.\n";
    }

    std::cout << "[Peer:" << port << "] Listening on port " << port << "\n";

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t   clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        if (clientSock < 0) { std::cerr << "[Peer:" << port << "] accept() failed\n"; continue; }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
        std::cout << "[Peer:" << port << "] Connection from " << clientIP << "\n";

        // Spawn a detached thread per connection
        std::thread([this, clientSock]() {
            this->handleClient(clientSock);
        }).detach();
    }
}

// ---------------------------------------------
//  Per-connection message loop
// ---------------------------------------------
void Peer::handleClient(int clientSocket) {
    while (true) {
        // Read 4-byte framing header
        uint32_t msgLen = 0;
        if (recvAll(clientSocket, (char*)&msgLen, sizeof(msgLen)) <= 0) {
            close(clientSocket); return;
        }
        msgLen = ntohl(msgLen);

        // Read body
        std::vector<char> body(msgLen);
        if (recvAll(clientSocket, body.data(), msgLen) <= 0) {
            close(clientSocket); return;
        }

        uint8_t msgType = static_cast<uint8_t>(body[0]);

        switch (msgType) {
            case MSG_PING: {
                std::cout << "[Peer:" << port << "] PING ? ACK\n";
                sendMessage(clientSocket, MSG_ACK, {});
                break;
            }
            case MSG_REQUEST_FILE_INFO:
                handleFileInfoRequest(clientSocket, body);
                break;
            case MSG_REQUEST_CHUNK:
                handleChunkRequest(clientSocket, body);
                break;
            default:
                std::cout << "[Peer:" << port << "] Unknown msg type: " << (int)msgType << "\n";
                break;
        }
    }
}

// ---------------------------------------------
//  Server-side: respond to MSG_REQUEST_FILE_INFO
//  body layout: [type 1][nameLen 2][filename N]
//  response:    MSG_FILE_INFO with [nameLen 2][name N][totalChunks 4][fileSize 4]
// ---------------------------------------------
void Peer::handleFileInfoRequest(int clientSocket, const std::vector<char>& body) {
    uint16_t nameLen;
    memcpy(&nameLen, body.data() + 1, sizeof(nameLen));
    nameLen = ntohs(nameLen);

    std::string filename(body.data() + 3, nameLen);
    std::string filepath = filesDir + "/" + filename;

    // Print the exact path being looked up to help diagnose "file not found" errors
    char cwd[4096];
    std::string absPath = filepath;
    if (getcwd(cwd, sizeof(cwd))) absPath = std::string(cwd) + "/" + filepath;
    std::cout << "[Peer:" << port << "] File info request: " << filename
              << "\n              Looking for: " << absPath << "\n";

    try {
        uint32_t totalChunks = FileManager::getChunkCount(filepath);
        uint32_t fileSize    = FileManager::getFileSize(filepath);

        std::vector<char> payload;
        uint16_t nlNet = htons(nameLen);
        payload.insert(payload.end(), (char*)&nlNet,       (char*)&nlNet + 2);
        payload.insert(payload.end(), filename.begin(),    filename.end());
        uint32_t chunksNet = htonl(totalChunks);
        payload.insert(payload.end(), (char*)&chunksNet,   (char*)&chunksNet + 4);
        uint32_t sizeNet = htonl(fileSize);
        payload.insert(payload.end(), (char*)&sizeNet,     (char*)&sizeNet + 4);

        sendMessage(clientSocket, MSG_FILE_INFO, payload);
        std::cout << "[Peer:" << port << "] Sent file info: "
                  << totalChunks << " chunk(s), " << fileSize << " bytes\n";
    } catch (const std::exception& e) {
        std::cerr << "[Peer:" << port << "] Error: " << e.what() << "\n";
    }
}

// ---------------------------------------------
//  Server-side: respond to MSG_REQUEST_CHUNK
//  body layout: [type 1][nameLen 2][filename N][chunkIndex 4]
//  response:    MSG_CHUNK_DATA with [chunkIndex 4][dataSize 4][data...]
// ---------------------------------------------
void Peer::handleChunkRequest(int clientSocket, const std::vector<char>& body) {
    uint16_t nameLen;
    memcpy(&nameLen, body.data() + 1, sizeof(nameLen));
    nameLen = ntohs(nameLen);

    std::string filename(body.data() + 3, nameLen);

    uint32_t chunkIndex;
    memcpy(&chunkIndex, body.data() + 3 + nameLen, sizeof(chunkIndex));
    chunkIndex = ntohl(chunkIndex);

    std::string filepath = filesDir + "/" + filename;
    std::cout << "[Peer:" << port << "] Chunk request: " << filename
              << " [" << chunkIndex << "]\n";

    try {
        std::vector<char> chunkData = FileManager::readChunk(filepath, chunkIndex);

        std::vector<char> payload;
        uint32_t idxNet      = htonl(chunkIndex);
        uint32_t dataSizeNet = htonl(static_cast<uint32_t>(chunkData.size()));
        payload.insert(payload.end(), (char*)&idxNet,      (char*)&idxNet + 4);
        payload.insert(payload.end(), (char*)&dataSizeNet, (char*)&dataSizeNet + 4);
        payload.insert(payload.end(), chunkData.begin(),   chunkData.end());

        sendMessage(clientSocket, MSG_CHUNK_DATA, payload);
        std::cout << "[Peer:" << port << "] Sent chunk [" << chunkIndex
                  << "] (" << chunkData.size() << " bytes)\n";
    } catch (const std::exception& e) {
        std::cerr << "[Peer:" << port << "] Error reading chunk: " << e.what() << "\n";
    }
}

// ---------------------------------------------
//  Client-side: send PING, wait for ACK
// ---------------------------------------------
void Peer::connectToPeer(const std::string& ip, uint16_t remotePort) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(remotePort);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cout << "[Peer] Failed to connect to " << ip << ":" << remotePort << "\n";
        return;
    }
    std::cout << "[Peer] Connected to " << ip << ":" << remotePort << "\n";

    sendMessage(sock, MSG_PING, {});

    // Wait for ACK before closing
    uint32_t ackLen = 0;
    uint8_t  ackType = 0;
    recvAll(sock, (char*)&ackLen,  sizeof(ackLen));
    recvAll(sock, (char*)&ackType, sizeof(ackType));
    std::cout << "[Peer] ACK received from " << ip << ":" << remotePort << "\n";

    close(sock);
}

// ---------------------------------------------
//  Client-side: download a file chunk by chunk
// ---------------------------------------------
void Peer::requestFile(const std::string& ip, uint16_t remotePort,
                       const std::string& filename, const std::string& savePath) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(remotePort);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cout << "[Peer] Failed to connect to " << ip << ":" << remotePort << "\n";
        return;
    }
    std::cout << "[Peer:" << port << "] Requesting '" << filename
              << "' from " << ip << ":" << remotePort << "\n";

    // -- Step 1: Ask for file metadata --
    {
        uint16_t nlNet = htons(static_cast<uint16_t>(filename.size()));
        std::vector<char> payload;
        payload.insert(payload.end(), (char*)&nlNet,    (char*)&nlNet + 2);
        payload.insert(payload.end(), filename.begin(), filename.end());
        sendMessage(sock, MSG_REQUEST_FILE_INFO, payload);
    }

    // -- Step 2: Receive file metadata --
    uint32_t totalChunks = 0;
    uint32_t fileSize    = 0;
    {
        uint32_t msgLen = 0;
        if (recvAll(sock, (char*)&msgLen, sizeof(msgLen)) <= 0) { close(sock); return; }
        msgLen = ntohl(msgLen);

        std::vector<char> body(msgLen);
        if (recvAll(sock, body.data(), msgLen) <= 0) { close(sock); return; }

        if (static_cast<uint8_t>(body[0]) != MSG_FILE_INFO) {
            std::cerr << "[Peer] Unexpected response to FILE_INFO request\n";
            close(sock); return;
        }

        // layout: [type 1][nameLen 2][name N][totalChunks 4][fileSize 4]
        uint16_t nameLen;
        memcpy(&nameLen, body.data() + 1, 2);
        nameLen = ntohs(nameLen);
        int offset = 1 + 2 + nameLen;

        memcpy(&totalChunks, body.data() + offset, 4);
        totalChunks = ntohl(totalChunks);
        offset += 4;

        memcpy(&fileSize, body.data() + offset, 4);
        fileSize = ntohl(fileSize);
    }
    std::cout << "[Peer:" << port << "] File has " << totalChunks
              << " chunk(s), " << fileSize << " byte(s) total\n";

    // -- Step 3: Request each chunk sequentially --
    for (uint32_t i = 0; i < totalChunks; i++) {
        // Send request
        {
            uint16_t nlNet  = htons(static_cast<uint16_t>(filename.size()));
            uint32_t idxNet = htonl(i);
            std::vector<char> payload;
            payload.insert(payload.end(), (char*)&nlNet,    (char*)&nlNet + 2);
            payload.insert(payload.end(), filename.begin(), filename.end());
            payload.insert(payload.end(), (char*)&idxNet,   (char*)&idxNet + 4);
            sendMessage(sock, MSG_REQUEST_CHUNK, payload);
        }

        // Receive chunk
        {
            uint32_t msgLen = 0;
            if (recvAll(sock, (char*)&msgLen, sizeof(msgLen)) <= 0) {
                std::cerr << "[Peer] Connection lost at chunk " << i << "\n";
                close(sock); return;
            }
            msgLen = ntohl(msgLen);

            std::vector<char> body(msgLen);
            if (recvAll(sock, body.data(), msgLen) <= 0) { close(sock); return; }

            if (static_cast<uint8_t>(body[0]) != MSG_CHUNK_DATA) {
                std::cerr << "[Peer] Unexpected response for chunk " << i << "\n";
                continue;
            }

            // layout: [type 1][chunkIndex 4][dataSize 4][data...]
            uint32_t recvIdx, dataSize;
            memcpy(&recvIdx,   body.data() + 1, 4); recvIdx   = ntohl(recvIdx);
            memcpy(&dataSize,  body.data() + 5, 4); dataSize  = ntohl(dataSize);

            std::vector<char> chunkData(body.begin() + 9, body.end());
            FileManager::writeChunk(savePath, recvIdx, chunkData);

            std::cout << "[Peer:" << port << "] ? Chunk [" << recvIdx
                      << "/" << (totalChunks - 1) << "] "
                      << chunkData.size() << " bytes\n";
        }
    }

    std::cout << "[Peer:" << port << "] ? Download complete ? " << savePath << "\n";
    close(sock);
}
