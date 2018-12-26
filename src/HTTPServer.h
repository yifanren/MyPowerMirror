/**
  httpserver
  HTTPServer.h
  Copyright 2011-2014 Ramsey Kant

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

#include <unordered_map>
#include <map>
//#include <vector>
#include <string>
#include <iostream>
#include <string.h>

#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>

#include "HTTPRequest.h"
#include "Client.h"

#define INVALID_SOCKET -1
#define QUEUE_SIZE 1024
#define CLIENTMAX 10

class HTTPServer {
    // Server Socket
    int listenPort;
    int listenSocket; // Descriptor for the listening socket
    struct sockaddr_in serverAddr; // Structure for the server address

    // select
    struct timeval timeout; // Block for 2 seconds and 0ns at the most
    int maxfd; // max descriptor
    //int evList[QUEUE_SIZE]; // Events that have triggered a filter in the kqueue (max QUEUE_SIZE at a time)
    fd_set read_fd; //set read_set

    //client
    std::unordered_map<int, Client*> clientMap;
    //int clientConnect[CLIENTMAX];

    // Connection processing
    void acceptConnection();
    void disconnectClient(Client* cl, bool mapErase = true);

    // read client
    void readClient(Client *cl);
    Client *getClient(int clfd);

    //void handleRequest(Client* cl, HTTPRequest* req);


    public:
    bool canRun;

    public:
    HTTPServer(int port);
    ~HTTPServer();

    bool start();
    void stop();

    // Main event loop
    void process();
};

#endif
