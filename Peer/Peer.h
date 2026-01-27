#ifndef PEER_H
#define PEER_H

class Peer{
    public:
        explicit Peer(int port);
        void start();
    private:
        int port;
        void startListener();
};
#endif