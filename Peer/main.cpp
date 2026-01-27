#include<iostream>
#include "Peer.h"

int main(int argc,char* argv[]){
    if(argc!=2){
        std::cerr<<"Usage: ./peer <port>\n";
        return 1;
    }
    int port = std::stoi(argv[1]);
    Peer peer(port);
    peer.start();
    return 0;
}