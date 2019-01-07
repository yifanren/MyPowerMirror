#include "HTTPServer.h"
#include "HTTPRequest.h"
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MESSAGE 99999

typedef struct requestInfo{
    int clientfd;
    char reqMessage[MESSAGE];
}request_t;



HTTPServer::HTTPServer(int port) {
    canRun = false;
    listenSocket = INVALID_SOCKET;
    listenPort = port;
    clientSocket = INVALID_SOCKET;
    //client[CLIENT_CONNECT] = {0};
    clientCon = 0;
}

HTTPServer::~HTTPServer() {

}

bool HTTPServer::start() {
    canRun = false;
    struct sockaddr_in serverAddr;

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(listenSocket == INVALID_SOCKET) {
        std::cout << "Could not create socket!" << std::endl;
        return false;
    }

    memset(&serverAddr, 0, sizeof(struct sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(listenPort);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if(bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
        std::cout << "Failed to bind to the address!" << std::endl;
        return false;
    }

    if(listen(listenSocket, SELISTEN_COUNT) != 0) {
        std::cout << "Failed to put the socket in a listening state" << std::endl;
        return false;
    }

    canRun = true;
    std::cout << "Server ready. Listening on port " << listenPort << "..." << std::endl;
    return true;
}

void HTTPServer::stop() {
    canRun = false;

    if(listenSocket != INVALID_SOCKET) {
        // Close all open connections and delete Client's from memory
        std::cout << "Server shutdown!" << std::endl;
    }
}

int HTTPServer::process() {
    int nev = 0; // Number of changed events
    fd_set readfd, tempfd;
    FD_SET(listenSocket, &readfd);
    int maxfd = listenSocket;
    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
	static request_t reqInfo;


    while (canRun) {
        FD_ZERO(&tempfd);
        tempfd = readfd;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        nev = select(maxfd + 1, &tempfd, NULL, NULL, &timeout);

        if (nev < 0) {
            printf("select failed\n");
            return 0;
        }
        else if (nev == 0) {
            printf("timeout\n");
            continue;
        }

        for (int i = 0; i < clientCon; i++) {
            if (FD_ISSET(client[i], &tempfd)) {
                memset(buffer, 0, BUFFER_SIZE);
                int len = recv(client[i], buffer, sizeof(buffer), 0);
                if (len <= 0) {
                    printf("client = %d close\n", client[i]);
                    close(client[i]);
                    FD_CLR(client[i], &readfd);
                    client[i] = 0;
                    clientCon--;

                    continue;
                }

                printf("buf = %s\n", buffer);
                reqInfo.clientfd = client[i];
                strcpy(reqInfo.reqMessage, buffer);
                //HTTPMessage(reqInfo);
            }
        }

        if (FD_ISSET(listenSocket, &tempfd)) {
            clientSocket = accept(listenSocket, (struct sockaddr*)&client_addr, &length);
            if (clientSocket < 0) {
                printf("accept failed\n");
                return 0;
            }
            client[clientCon++] = clientSocket;
            FD_SET(clientSocket, &readfd);
            if (clientSocket > maxfd)
                maxfd = clientSocket;

            printf("%d new client connect\n", clientSocket);
        }
    }
}
