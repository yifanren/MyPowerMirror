#include "HTTPServer.h"
#define DATA_LEN 5555

HTTPServer::HTTPServer(int port) {
    canRun = false;
    listenSocket = INVALID_SOCKET;
    listenPort = port;
    maxfd = -1;
}

HTTPServer::~HTTPServer() {

}

bool HTTPServer::start() {
    canRun = false;

    // Create a handle for the listening socket, TCP
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(listenSocket == INVALID_SOCKET) {
        std::cout << "Could not create socket!" << std::endl;
        return false;
    }

    // Populate the server address structure
    // modify to support multiple address families (bottom): http://eradman.com/posts/kqueue-tcp.html
    memset(&serverAddr, 0, sizeof(struct sockaddr_in)); // clear the struct
    serverAddr.sin_family = AF_INET; // Family: IP protocol
    serverAddr.sin_port = htons(listenPort); // Set the port (convert from host to netbyte order)
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Let OS intelligently select the server's host address

    // Bind: Assign the address to the socket
    if(bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
        std::cout << "Failed to bind to the address!" << std::endl;
        return false;
    }

    // Listen: Put the socket in a listening state, ready to accept connections
    // Accept a backlog of the OS Maximum connections in the queue
    if(listen(listenSocket, SOMAXCONN) != 0) {
        std::cout << "Failed to put the socket in a listening state" << std::endl;
        return false;
    }

    //set Socket to fd_set
    FD_ZERO(&read_fd);
    FD_SET(listenSocket, &read_fd);
    maxfd = listenSocket;

    canRun = true;
    std::cout << "Server ready. Listening on port " << listenPort << "..." << std::endl;
    return true;
}

/**
 * Stop
 * Disconnect all clients and cleanup all server resources created in start()
 */
void HTTPServer::stop() {
    canRun = false;
    std::cout << "Server shutdown!" << std::endl;
}

/**
 * Server Process
 * Main server processing function that checks for any new connections or data to read on
 * the listening socket
 */
void HTTPServer::process() {
    int nev = 0; // Number of changed events returned by kevent
    Client* cl = NULL;
    fd_set tempSet;
    std::unordered_map<int, Client*>::iterator it;
    while(1) {
        FD_ZERO(&tempSet);
        tempSet = read_fd;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        // Get a list of changed socket descriptors with a read event triggered in evList
        // Timeout set in the header
        nev = select(maxfd + 1, &tempSet, NULL, NULL, &timeout);
		int curFd = 0;
        if (nev < 0) {
            printf("select failed\n");
            break;
        }

        if(nev == 0) {
            printf("timeout\n");	
            continue;
        }
        printf("have info...\n");
		for ( it = clientMap.begin(); it != clientMap.end(); it++) {
            if (FD_ISSET(it->first, &tempSet)) {
                printf("client %d send request\n", it->first);
                cl = it->second;
                printf("ip [%s] send request\n", cl->getClientIP());
                readClient(cl);
            }
        }
        if (FD_ISSET(listenSocket, &tempSet)) {
            acceptConnection();
            printf("connect end ..\n");
        }
    } // canRun
}
/**
 * Accept Connection
 * When a new connection is detected in runServer() this function is called. This attempts to accept the pending 
 * connection, instance a Client object, and add to the client Map
 */
void HTTPServer::acceptConnection() {
    // Setup new client with prelim address info
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    int clfd = INVALID_SOCKET;
    //int begin = 0;
    //int i = 0;

    // Accept the pending connection and retrive the client descriptor
    clfd = accept(listenSocket, (sockaddr*)&clientAddr, (socklen_t*)&clientAddrLen);
    if(clfd == INVALID_SOCKET)
        return;

    // Instance Client object
    Client *cl = new Client(clfd, clientAddr);

    // Add the client object to the client map
    clientMap.insert(std::pair<int, Client*>(clfd, cl));

    // Select maxfd
    FD_SET(clfd, &read_fd);
	if (maxfd < clfd)
        maxfd = clfd;

    // Print the client's IP on connect
    std::cout << "[" << cl->getClientIP() << "] connected" << std::endl;

    // Map size
    std::cout << "map.size() is " << clientMap.size() << std::endl;
}

/**
 * Disconnect Client
 * Close the client's socket descriptor and release it from the FD map, client map, and memory
 *
 * @param cl Pointer to Client object
 * @param mapErase When true, remove the client from the client map. Needed if operations on the
 * client map are being performed and we don't want to remove the map entry right away
 */
void HTTPServer::disconnectClient(Client *cl, bool mapErase) {
    if(cl == NULL)
        return;

    std::cout << "[" << cl->getClientIP() << "] disconnected" << std::endl;

    // Remove socket events from select
    FD_CLR(cl->getSocket(), &read_fd);

    // Close the socket descriptor
    close(cl->getSocket());

    // Remove the client from the clientMap
    if(mapErase)
        clientMap.erase(cl->getSocket());

    // Delete the client object from memory
    delete cl;
}

/**
 * Read Client
 * Recieve data from a client that has indicated that it has data waiting. Pass recv'd data to handleRequest()
 * Also detect any errors in the state of the socket
 *
 * @param cl Pointer to Client that sent the data
 * @param data_len Number of bytes waiting to be read
 */
void HTTPServer::readClient(Client *cl) {
    // Receive data on the wire into pData
    /* TODO: Figure out what flags need to be set */
    int flags = 0;
    int i = 0;

    //HTTPRequest* req;
    //char* pData = new char[DATA_LEN];
    char pData[DATA_LEN];
    memset(pData, 0, DATA_LEN);
    ssize_t lenRecv = recv(cl->getSocket(), pData, DATA_LEN, flags);

    // Determine state of the client socket and act on it
    if(lenRecv <= 0) {
        // Client closed the connection
        disconnectClient(cl, true);
    } else {
        // Data received: Place the data in an HTTPRequest and pass it to handleRequest for processing
        //req = new HTTPRequest((byte *)pData, lenRecv);
        //handleRequest(clifd, req);
        //delete req;
        printf("request handle end...\n");
    }
}

Client* HTTPServer::getClient(int clfd) {
    auto it = clientMap.find(clfd);

    // Client wasn't found
    if(it == clientMap.end())
        return NULL;

    // Return a pointer to the client object
    return it->second;
}

