#include "Peer.h"
#include "Protocol.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>

// Usage:
//   Seeder  (serves files): ./peer 5002 <files_dir>
//   Downloader (downloads): ./peer 5001 <save_dir>
//
// Demo flow:
//   1. Start seeder:     ./peer 5002 files_5002
//   2. Start downloader: ./peer 5001 .
//      ? peer 5001 requests "test.txt" from 5002 and saves as "received_test.txt"
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./peer <port> [files_dir]\n";
        return 1;
    }

    int port = std::stoi(argv[1]);
    std::string filesDir = (argc >= 3) ? argv[2] : ".";

    Peer peer(port, filesDir);

    // Start the listener in a background thread
    std::thread listenerThread([&peer]() {
        peer.startListener();
    });
    listenerThread.detach();

    // Give the listener a moment to bind and start accepting
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (port == 5001) {
        // Downloader: request test.txt from the seeder at 5002
        peer.requestFile("127.0.0.1", 5002, "test.txt", "received_test.txt");
    }

    // Keep the process alive so the listener thread keeps running
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}
