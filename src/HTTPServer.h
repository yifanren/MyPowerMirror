#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

class HTTPServer{

    bool canRun;
    int listenPort;
    int listenSocket;
    int httpServerFd;
    struct timeval timeout;
    struct sockaddr_in serverAddr;

    public:
        HTTPServer(char *host, int port);
        ~HTTPServer();

        bool start();
        void stop();

        void process();
}

#endif
