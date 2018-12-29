#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

#include <unordered_map>
#include <map>
#include <vector>
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

#include "HTTPMessage.h"
#include "ResourceHost.h"
#include "HTTPResponse.h"
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
    fd_set read_fd; //set read_set

    //client
    std::unordered_map<int, Client*> clientMap;

	// Resources / File System
	std::vector<ResourceHost*> hostList; // Contains all ResourceHosts
	std::unordered_map<std::string, ResourceHost*> vhosts; // Virtual hosts. Maps a host string to a ResourceHost to service the request


    // Connection processing
    void acceptConnection();
    void disconnectClient(Client* cl, bool mapErase = true);

    // read client
    void readClient(Client *cl);
    Client *getClient(int clfd);

    //void handleRequest(Client* cl, HTTPRequest* req);
    void handleRequest(Client* cl, HTTPRequest* req);
	void handleGet(Client* cl, HTTPRequest* req, ResourceHost* resHost);
	void handlePOST(Client* cl, HTTPRequest* req);
	void handleOptions(Client* cl, HTTPRequest* req);
	void handleTrace(Client* cl, HTTPRequest* req);


    // Response
	void sendStatusResponse(Client* cl, int status, std::string msg = "");
	void sendResponse(Client* cl, HTTPResponse* resp, bool disconnect);

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
