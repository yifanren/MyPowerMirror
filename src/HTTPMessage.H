#ifndef _HTTPMESSAGE_H_
#define _HTTPMESSAGE_H_

#define DEFAULT_HTTP_VERSION "HTTP/1.1"

enum Method {
    HEAD = 0,
    GET  = 1,
    POST = 2,
    PUT  = 3
};

enum Status {
    CONTINUE = 100,
    OK       = 200,

    BAD_REQUEST = 400,
    NOT_FOUND   = 404,

    SERVER_ERROR    = 500,
    NOT_IMPLEMENTED = 501
};

class HTTPMessage {
    public:
        HTTPMessage();
        ~HTTPMessage();

        int readLine(char *buf);



}



#endif
