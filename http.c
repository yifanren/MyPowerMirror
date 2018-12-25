#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "include/util.h"
#include "include/http.h"

#define LINE 20

Request parseRequest(char * buffer)
{
    Request req;
    char *head, *temp, *requestLine;
    char *lineHead[LINE];
    int line = 0;

    req.accept = NULL;
    req.accept_Encoding = NULL;
    req.accept_Language = NULL;
    req.connection = NULL;
    req.content_Length = NULL;
    req.host = NULL;
    req.method = NULL;
    req.file = NULL;
    req.user_Agent = NULL;
    req.Expect = NULL;

    requestLine = strtok(buffer, "\n");
    while(requestLine) {
        lineHead[line++] = requestLine;
        requestLine = strtok(NULL, "\n");
        if (indexOfStrFirst(requestLine, '\r') == 0)
            break;
    }

    req.method = strtok(lineHead[0], " ");
    if (strcmp(req.method, "POST")) {
        printf("method = %s\n", req.method);
    } else if (strcmp(req.method, "GET")) {
        printf("method = %s\n", req.method);
    } else {
        printf("method wrong\n");
        return req;
    }

    for (int i = 1 ; i < line; i++)
        parseHeader(&req, lineHead[i]);

    return req;
}

void parseHeader(Request *req, char *head)
{
    char *temp;
    temp = strtok(head, " ");
    //printf("temp = %s\n", temp);
    if (strcmp(head, "Host:")==0) {
        temp = strtok(NULL, " ");
        req->host = (char *)malloc((strlen(temp) + 1));
        strcpy(req->host, temp);
    }
    else if (strcmp(head, "User-Agent:")==0) {
        temp = strtok(NULL, " ");
        req->user_Agent = (char *)malloc((strlen(temp) + 1));
        strcpy(req->user_Agent, temp);
    }
    else if (strcmp(head, "Accept:")==0) {
        temp = strtok(NULL, " ");
        req->accept = (char *)malloc((strlen(temp) + 1));
        strcpy(req->accept, temp);
    }
    else if (strcmp(head, "Accept-Language:")==0) {
        temp = strtok(NULL, " ");
        req->accept_Language = (char *)malloc((strlen(temp) + 1));
        strcpy(req->accept_Language, temp);
    }
    else if (strcmp(head, "Accept-Encoding:")==0) {
        temp = strtok(NULL, " ");
        req->accept_Encoding = (char *)malloc((strlen(temp) + 1));
        strcpy(req->accept_Encoding, temp);
    }
    else if (strcmp(head, "Content-Length:")==0) {
        temp = strtok(NULL, " ");
        req->content_Length = (char *)malloc((strlen(temp) + 1));
        strcpy(req->content_Length, temp);
    }
    else if(strcmp(head, "Content-type:")==0) {
        temp = strtok(NULL, " ");
        req->content_Type= (char *)malloc((strlen(temp) + 1));
        strcpy(req->content_Type, temp);
    }
    else if(strcmp(head, "Connection:") == 0) {
        temp = strtok(NULL, " ");
        req->connection = (char *)malloc((strlen(temp) + 1));
        strcpy(req->connection, temp);
    }
    else if(strcmp(head, "Expect:") == 0) {
        temp = strtok(NULL, " ");
        req->Expect = (char *)malloc((strlen(temp) + 1));
        strcpy(req->Expect, temp);
    }
}
