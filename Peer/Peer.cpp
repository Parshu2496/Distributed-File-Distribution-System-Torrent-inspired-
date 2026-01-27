#include "ConnectionHandler.h"
#include <vector>
#include "Peer.h"
#include <arpa/inet.h> //nthol
#include <unistd.h>
// Unix system calls like close()
#include <iostream>
#include <cstdint> //uint32_t, uint8_t
#include <thread>
#include <netinet/in.h>
// Network structure for internet sockets
#include <cstring>
#include "Protocol.h"
using namespace std;
Peer::Peer(int port) : port(port) {}
// Peer constructor is called by passing the port number same as this->port = port;
void Peer::start()
{
    cout << "Peer starting at port" << port << endl;
    thread listenerThread(&Peer::startListener, this);
    // Creates a new thread by passing the function startListener and current peer object(this)
    listenerThread.join();
    //.join means wait for the thread to finish but startListener() runs forever so join() blocks everything
}

void sendAck(int socket)
{
    uint8_t type = MSG_ACK;
    uint32_t length = htonl(1);

    send(socket, &length, sizeof(length), 0);
    send(socket, &type, sizeof(type), 0);
}

void Peer::startListener()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    // create a socket and passes the value AF_INET = Address Family = IPV4, SOCK_STREAM= TCP,0 = let OS choose default protocol for TCP
    if (serverSocket < 0)
    {
        cerr << "Failed to create socket";
        return;
    }
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;         // IPV4
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces
    serverAddr.sin_port = htons(port);       // Converts port into network format.
    // A sockaddr_in is a structure tha holds IP address, port and protocol family. {} initialize everything to zero
    if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    { // Binding sockets to IP and Port
        cerr << "Bind failed\n";
        close(serverSocket);
        return;
    }
    if (listen(serverSocket, 10) < 0)
    { // Ready to accept incoming TCP connections. 10 max pending connections OS can queue
        cerr << "Listen failed\n";
        close(serverSocket);
        return;
    }
    cout << "Listening for connections\n";
    while (true)
    {                             // infinite loop
        sockaddr_in clientAddr{}; // Will store clients IP and port. OS fills this when client connect
        socklen_t clientLen = sizeof(clientAddr);

        int clientSocket = accept(serverSocket, (sockaddr *)&clientAddr, &clientLen); // accept blocks the code until client connects. Creates a new socket. Return clientSocket
        if (clientSocket < 0)
        {
            cerr << "Accept failed\n";
            continue;
        }
        cout << "Accepted new connnection\n";

        thread handlerThread([clientSocket]() { // Create a new thread. Run the code inside {} in parallel. Capture clientSocket for this thread
            // char buffer[1024];//buffer to store the received bytes
            // int bytes = recv(clientSocket,buffer,sizeof(buffer),0);//receive data from client

            // if(bytes>0){
            //     cout<<"Received "<<bytes<<" bytes\n";
            // }
            // close(clientSocket);
            while (true)
            {
                // STEP 1 Read Header(4bytes)
                uint32_t messageLength = 0;
                int result = recvAll(
                    clientSocket,
                    (char *)&messageLength,
                    sizeof(messageLength));
                if (result <= 0)
                {
                    close(clientSocket);
                    return;
                }
                // Convert from network byte order to host byte ordeer
                messageLength = ntohl(messageLength); // turn the messageLength from small endia to Big endia
                // Big-endian stores the most significant byte (MSB) at the lowest memory address, aligning with left-to-right reading order (e.g., used in network protocols like TCP/IP, known as "network byte order").
                // Little-endian stores the least significant byte (LSB) at the lowest memory address, commonly used in x86 and ARM processors.

                // STEP 2 read body(messageLength byte)
                vector<char> messageBuffer(messageLength);
                result = recvAll(
                    clientSocket,
                    messageBuffer.data(),
                    messageLength);

                if (result <= 0)
                {
                    break;
                }

                // STEP 3 Parse body
                uint8_t messageType = static_cast<uint8_t>(messageBuffer[0]);
                int payloadSize = messageLength - 1;
                cout << "\nReceived messageType = "
                     << static_cast<int>(messageType)
                     << "\nPayloadSize =  "
                     << payloadSize
                     << endl;
                if (messageType == MSG_PING)
                    cout << "Ping received\n";
            }
            close(clientSocket);
        });
        handlerThread.detach(); // I don’t care when this thread finishes. Just let it run on its own.
    }
}