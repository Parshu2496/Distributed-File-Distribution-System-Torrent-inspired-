#ifndef PEER_H
#define PEER_H
#include <string>
#include <vector>
#include <cstdint>

class Peer {
public:
    // port     — TCP port this peer listens on
    // filesDir — directory from which files are served to other peers
    explicit Peer(int port, const std::string& filesDir = ".");

    void start();         // blocking: starts listener (joins thread)
    void startListener(); // accept loop — runs forever

    // Send a PING to another peer and wait for ACK
    void connectToPeer(const std::string& ip, uint16_t remotePort);

    // Download filename from ip:remotePort and save to savePath
    void requestFile(const std::string& ip, uint16_t remotePort,
                     const std::string& filename,
                     const std::string& savePath);

private:
    int port;
    std::string filesDir;

    // Handles all incoming messages for one connected client socket
    void handleClient(int clientSocket);

    // Server-side handlers (called from handleClient)
    void handleFileInfoRequest(int clientSocket, const std::vector<char>& body);
    void handleChunkRequest(int clientSocket, const std::vector<char>& body);

    // Frames and sends a message: [4-byte length][type][payload]
    void sendMessage(int sock, uint8_t type, const std::vector<char>& payload);
};
#endif
