#include <iostream>
#include <thread>
#include <vector>
#include <arpa/inet.h>
#include <unistd.h>

#include "Protocol.h"
#include "ConnectionHandler.h"
#include <unordered_map>
#include <mutex>
#include <chrono>
#include<cstring>
using namespace std;

struct PeerInfo
{
    std::string ip;
    uint16_t port;
    std::chrono::steady_clock::time_point lastSeen;
};
std::unordered_map<std::string, PeerInfo> peers;
std::mutex peersMutex;
std::string makePeerKey(const std::string &ip, uint16_t port)
{
    return ip + ":" + std::to_string(port);
}
std::vector<char> buildPeerListPayload() {
    std::vector<char> payload;

    std::lock_guard<std::mutex> lock(peersMutex);

    uint32_t count = peers.size();
    uint32_t countNet = htonl(count);

    payload.insert(payload.end(),
                   (char*)&countNet,
                   (char*)&countNet + sizeof(countNet));

    for (const auto& [key, peer] : peers) {
        uint32_t ip = inet_addr(peer.ip.c_str());
        uint16_t port = htons(peer.port);

        payload.insert(payload.end(),
                       (char*)&ip,
                       (char*)&ip + sizeof(ip));

        payload.insert(payload.end(),
                       (char*)&port,
                       (char*)&port + sizeof(port));
    }

    return payload;
}
void handleClient(int clientSocket)
{
    while (true)
    {
        uint32_t messageLength = 0;
        int result = recvAll(
            clientSocket,
            (char *)&messageLength,
            sizeof(messageLength));
        if (result <= 0)
            break;

        messageLength = ntohl(messageLength);

        std::vector<char> messageBuffer(messageLength);
        result = recvAll(
            clientSocket,
            messageBuffer.data(),
            messageLength);
        if (result <= 0)
            break;

        uint8_t MessageType = messageBuffer[0];

        if (MessageType == MSG_REGISTER)
        {

            uint32_t peerPortNet;
            memcpy(&peerPortNet, messageBuffer.data() + 1, sizeof(peerPortNet));
            uint16_t peerPort = ntohl(peerPortNet);

            sockaddr_in addr;
            socklen_t len = sizeof(addr);
            getpeername(clientSocket, (sockaddr *)&addr, &len);

            std::string peerIp = inet_ntoa(addr.sin_addr);

            PeerInfo info;
            info.ip = peerIp;
            info.port = peerPort;
            info.lastSeen = std::chrono::steady_clock::now();

            std::string key = makePeerKey(peerIp, peerPort);

            {
                std::lock_guard<std::mutex> lock(peersMutex);
                peers[key] = info;
            }

            std::cout << "[Tracker] Registered peer " << key << std::endl;
        }
        else if (MessageType == MSG_HEARTBEAT)
        {

            sockaddr_in addr;
            socklen_t len = sizeof(addr);
            getpeername(clientSocket, (sockaddr *)&addr, &len);

            std::string peerIp = inet_ntoa(addr.sin_addr);
            uint16_t peerPort = ntohs(addr.sin_port);

            std::string key = makePeerKey(peerIp, peerPort);

            {
                std::lock_guard<std::mutex> lock(peersMutex);
                if (peers.count(key))
                {
                    peers[key].lastSeen = std::chrono::steady_clock::now();
                }
            }

            std::cout << "[Tracker] HEARTBEAT updated for " << key << std::endl;
        }
        else if (MessageType == MSG_GET_PEERS) {

    std::vector<char> payload = buildPeerListPayload();

    uint32_t length = htonl(1 + payload.size());
    uint8_t type = MSG_PEER_LIST;

    send(clientSocket, &length, sizeof(length), 0);
    send(clientSocket, &type, sizeof(type), 0);
    send(clientSocket, payload.data(), payload.size(), 0);

    std::cout << "[Tracker] Sent peer list ("
              << payload.size() << " bytes)"
              << std::endl;
}
        // Send ACK
        uint32_t len = htonl(1);
        uint8_t ack = MSG_ACK;
        send(clientSocket, &len, sizeof(len), 0);
        send(clientSocket, &ack, sizeof(ack), 0);
    }
    close(clientSocket);
}
int main()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(7000);

    bind(serverSocket, (sockaddr *)&addr, sizeof(addr));
    listen(serverSocket, 10);
    cout << "[Tracker] Listening on port 7000\n";
    std::thread cleanupThread([]() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));

        auto now = std::chrono::steady_clock::now();

        std::lock_guard<std::mutex> lock(peersMutex);

        for (auto it = peers.begin(); it != peers.end(); ) {
            auto diff = std::chrono::duration_cast<std::chrono::seconds>(
                now - it->second.lastSeen
            ).count();

            if (diff > 30) {
                std::cout << "[Tracker] Removing dead peer "
                          << it->first << std::endl;
                it = peers.erase(it);
            } else {
                ++it;
            }
        }
    }
});
cleanupThread.detach();
    while (true)
    {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        thread(handleClient, clientSocket).detach();
    }
}
