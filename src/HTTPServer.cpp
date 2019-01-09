#include "HTTPServer.h"
#define DATA_LEN 555555

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

        for ( it = clientMap.begin(); it != clientMap.end(); it++) {
            if (FD_ISSET(it->first, &tempSet)) {
                cl = it->second;
                readClient(cl);
            }
        }
        if (FD_ISSET(listenSocket, &tempSet)) {
            acceptConnection();
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
    ssize_t len = 0;
    std::string hlenstr = " ";

    HTTPRequest* req;
    char* pData = new char[DATA_LEN];
    memset(pData, 0, DATA_LEN);
    ssize_t lenRecv = recv(cl->getSocket(), pData, DATA_LEN, flags);

    // Determine state of the client socket and act on it
    if(lenRecv <= 0) {
        // Client closed the connection
        disconnectClient(cl, true);
    } else {
        if (strstr(pData, "Expect: 100-continue")) {
            printf("Expect: 100-continue here ...\n");
            req = new HTTPRequest((byte *)pData, lenRecv);
            req->parse();

            hlenstr = req->getHeaderValue("Content-Length");
            int contLen = atoi(hlenstr.c_str());
            ssize_t temp = 0;
            char* buff = new char[DATA_LEN];
            while (1) {
                memset(buff, 0, DATA_LEN);
                len = recv(cl->getSocket(), buff, DATA_LEN, 0);
                temp += len;
                strcat(pData, buff);

                if (temp == contLen)
                    break;
            }
            lenRecv += temp;
            delete [] buff;
        }

        // Data received: Place the data in an HTTPRequest and pass it to handleRequest for processing
        req = new HTTPRequest((byte *)pData, lenRecv);
        handleRequest(cl, req);
        delete req;
        delete [] pData;			
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

/**
 * Handle Request
 * Process an incoming request from a Client. Send request off to appropriate handler function
 * that corresponds to an HTTP operation (GET, HEAD etc) :)
 *
 * @param cl Client object where request originated from
 * @param req HTTPRequest object filled with raw packet data
 */
void HTTPServer::handleRequest(Client *cl, HTTPRequest* req) {
    // Parse the request
    // If there's an error, report it and send a server error in response
    if(!req->parse()) {
        std::cout << "[" << cl->getClientIP() << "] There was an error processing the request of type: " << req->methodIntToStr(req->getMethod()) << std::endl;
        std::cout << req->getParseError() << std::endl;
        sendStatusResponse(cl, Status(BAD_REQUEST), req->getParseError());
        return;
    }

    std::cout << "[" << cl->getClientIP() << "] " << "send request" << std::endl;
    std::cout << "Headers:" << std::endl;
    for(int i = 0; i < req->getNumHeaders(); i++) {
        std::cout << req->getHeaderStr(i) << std::endl;
    }
    std::cout << std::endl;

    // Determine the appropriate vhost
    std::string host = "";

    // Retrieve the host specified in the request (Required for HTTP/1.1 compliance)
    if(req->getVersion().compare(HTTP_VERSION_11) == 0) {
        host = req->getHeaderValue("Host");
    }

    // Send the request to the correct handler function
    switch(req->getMethod()) {
        case Method(POST):
            handlePOST(cl, req);
            break;
        case Method(HEAD):
        case Method(GET):
            //handleGet(cl, req, resHost);
            break;
        case Method(OPTIONS):
            handleOptions(cl, req);
            break;
        case Method(TRACE):
            handleTrace(cl, req);
            break;
        default:
            std::cout << "[" << cl->getClientIP() << "] Could not handle or determine request of type " << req->methodIntToStr(req->getMethod()) << std::endl;
            sendStatusResponse(cl, Status(NOT_IMPLEMENTED));
            break;
    }
}

/**
 * Handle Get or Head
 * Process a GET or HEAD request to provide the client with an appropriate response
 *
 * @param cl Client requesting the resource
 * @param req State of the request
 * @param resHost Resource host to service the request
 */
void HTTPServer::handleGet(Client* cl, HTTPRequest* req, ResourceHost* resHost) {	
    // Check if the requested resource exists
    std::string uri = req->getRequestUri();
    Resource* r = resHost->getResource(uri);

    if(r != NULL) { // Exists
        HTTPResponse* resp = new HTTPResponse();
        resp->setStatus(Status(OK));
        resp->addHeader("Content-Type", r->getMimeType());
        resp->addHeader("Content-Length", r->getSize());

        // Only send a message body if it's a GET request. Never send a body for HEAD
        if(req->getMethod() == Method(GET))
            resp->setData(r->getData(), r->getSize());

        bool dc = false;

        // HTTP/1.0 should close the connection by default
        if(req->getVersion().compare(HTTP_VERSION_10) == 0)
            dc = true;

        // If Connection: close is specified, the connection should be terminated after the request is serviced
        std::string connection_val = req->getHeaderValue("Connection");
        if(connection_val.compare("close") == 0)
            dc = true;

        sendResponse(cl, resp, dc);
        delete resp;
        delete r;
    } else { // Not found
        sendStatusResponse(cl, Status(NOT_FOUND));
    }
}

/**
 * Handle POST
 * Process a POST request to provide the client with an appropriate response
 *
 * @param cl Client requesting the resource
 * @param req State of the request
 * @param resHost Resource host to service the request
 */
void HTTPServer::handlePOST(Client* cl, HTTPRequest* req) {	
    // Check if the requested resource exists
    HTTPResponse* resp = new HTTPResponse();
    resp->setStatus(Status(OK));
    resp->addHeader("Content-Type", "text/html");
    resp->addHeader("Content-Length", "0");

    bool dc = false;

    // HTTP/1.0 should close the connection by default
    if(req->getVersion().compare(HTTP_VERSION_11) == 0)
        dc = true;

    // If Connection: close is specified, the connection should be terminated after the request is serviced
    sendResponse(cl, resp, dc);
}


/**
 * Handle Options
 * Process a OPTIONS request
 * OPTIONS: Return allowed capabilties for the server (*) or a particular resource
 *
 * @param cl Client requesting the resource
 * @param req State of the request
 */
void HTTPServer::handleOptions(Client* cl, HTTPRequest* req) {
    // For now, we'll always return the capabilities of the server instead of figuring it out for each resource
    std::string allow = "POST, HEAD, GET, OPTIONS, TRACE";

    HTTPResponse* resp = new HTTPResponse();
    resp->setStatus(Status(OK));
    resp->addHeader("Allow", allow.c_str());
    resp->addHeader("Content-Length", "0"); // Required

    sendResponse(cl, resp, true);
    delete resp;
}

/**
 * Handle Trace
 * Process a TRACE request
 * TRACE: send back the request as received by the server verbatim
 *
 * @param cl Client requesting the resource
 * @param req State of the request
 */
void HTTPServer::handleTrace(Client* cl, HTTPRequest *req) {
    // Get a byte array representation of the request
    unsigned int len = req->size();
    byte* buf = new byte[len];
    req->setReadPos(0); // Set the read position at the beginning since the request has already been read to the end
    req->getBytes(buf, len);

    // Send a response with the entire request as the body
    HTTPResponse* resp = new HTTPResponse();
    resp->setStatus(Status(OK));
    resp->addHeader("Content-Type", "message/http");
    resp->addHeader("Content-Length", len);
    resp->setData(buf, len);
    sendResponse(cl, resp, true);

    delete resp;
    delete[] buf;
}

/**
 * Send Status Response
 * Send a predefined HTTP status code response to the client consisting of
 * only the status code and required headers, then disconnect the client
 *
 * @param cl Client to send the status code to
 * @param status Status code corresponding to the enum in HTTPMessage.h
 * @param msg An additional message to append to the body text
 */
void HTTPServer::sendStatusResponse(Client* cl, int status, std::string msg) {
    HTTPResponse* resp = new HTTPResponse();
    resp->setStatus(Status(status));

    // Body message: Reason string + additional msg	
    std::string body = resp->getReason() + ": " + msg;
    unsigned int slen = body.length();
    char* sdata = new char[slen];
    strncpy(sdata, body.c_str(), slen);

    resp->addHeader("Content-Type", "text/plain");
    resp->addHeader("Content-Length", slen);
    resp->setData((byte*)sdata, slen);

    sendResponse(cl, resp, true);

    delete resp;
}

/**
 * Send Response
 * Send a generic HTTPResponse packet data to a particular Client
 *
 * @param cl Client to send data to
 * @param buf ByteBuffer containing data to be sent
 * @param disconnect Should the server disconnect the client after sending (Optional, default = false)
 */
void HTTPServer::sendResponse(Client* cl, HTTPResponse* resp, bool disconnect) {
    // Server Header
    //resp->addHeader("Server", "httpserver/1.1");
    // Time stamp the response with the Date header
    std::string tstr;
    char tbuf[1024];

    if(disconnect)
        resp->addHeader("Connection", "close");
    // Get raw data by creating the response (we are responsible for cleaning it up in process())
    byte* pData = resp->create();
    strcpy(tbuf, (char*)pData);
    // Add data to the Client's send queue
    cl->addToSendQueue(new SendQueueItem(pData, resp->size(), disconnect));

    // Send data to client
    send(cl->getSocket(), tbuf, strlen(tbuf), 0);
}

