#include<iostream>
#include<thread>
#include<vector>
#include<arpa/inet.h>
#include<unistd.h>

#include "Protocol.h"
#include "ConnectionHandler.h"
using namespace std;
void handleClient(int clientSocket){
    while (true)
    {
        uint32_t messageLength = 0;
        int result = recvAll(
            clientSocket,
            (char*)&messageLength,
            sizeof(messageLength)
        );
        if(result<=0) break;

        messageLength = ntohl(messageLength);

        std::vector<char> messageBuffer(messageLength);
        result = recvAll(
            clientSocket,
            messageBuffer.data(),
            messageLength
        );
        if(result<=0) break;

        uint8_t MessageType = messageBuffer[0];

        if(MessageType==MSG_REGISTER){
            cout<<"[Tracker] Register received\n";
        }
        else if(MessageType==MSG_HEARTBEAT){
            cout<<"[Tracker] HEARTBEAT received\n";
        }
        else if(MessageType==MSG_GET_PEERS){
            cout<<"[Tracker] GET PEERS received\n";
        }
        // Send ACK
        uint32_t len = htonl(1);
        uint8_t ack = MSG_ACK;
        send(clientSocket,&len,sizeof(len),0);
        send(clientSocket,&ack,sizeof(ack),0);
    }
    close(clientSocket);
    
}
int main(){
    int serverSocket = socket(AF_INET,SOCK_STREAM,0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(7000);

    bind(serverSocket,(sockaddr*)&addr,sizeof(addr));
    listen(serverSocket,10);
    cout <<"[Tracker] Listening on port 7000\n";

    while(true){
        int clientSocket = accept(serverSocket,nullptr,nullptr);
        thread(handleClient,clientSocket).detach();
    }
}
