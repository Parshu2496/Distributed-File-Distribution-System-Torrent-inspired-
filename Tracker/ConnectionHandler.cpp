#include "ConnectionHandler.h"
#include <vector>
#include "Peer.h"
#include <arpa/inet.h>//nthol
#include <unistd.h>
// Unix system calls like close()
#include <iostream>
#include<cstdint> //uint32_t, uint8_t
#include <thread>
#include <netinet/in.h>
// Network structure for internet sockets
#include <cstring>
#include <sys/socket.h>

int recvAll(int socket,char* buffer,int length){
    int totalReceived = 0;
    while(totalReceived<length){
        int bytes = recv(
            socket,//receive on client socket 
            buffer+totalReceived,//Points to a buffer where the message should be stored.
            length-totalReceived,//specifies the length in bytes of the buffer pointed to by the buffer argument.
            0//Specifies the type of message reception
        );
        if(bytes<=0){//after the size of bytes is received the recv() func return -1 
            close(socket);
            return -1;
        }
        totalReceived+=bytes;
    }
    return totalReceived;
}