#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>

#include "include/curl/curl.h"
#include "include/util.h"
#include "include/http.h"

#define PORT         1239
#define BUFFER_SIZE  250000
#define LISTEN_COUNT 10

//处理用户的连接的线程方法
void response(Request res, int connect){
    char *buf = NULL;
    char temp[BUFFER_SIZE];
    int fp = -1;

    //发送状态码  (用字符串链接)
    int len = strlen("HTTP/1.1 200 OK\r\nConnection: close\r\nAccept-Ranges: bytes\r\n")+1;
    buf = (char *)malloc(sizeof(char)*len);
    strcpy(buf,"HTTP/1.1 200 OK\r\nConnection: close\r\nAccept-Ranges: bytes\r\n");
    sprintf(temp,"Content-Type: %s\r\n", getFileType(res.content_Type));
    len += strlen(temp);
    buf = (char *)realloc(buf,sizeof(char)*len);
    strcat(buf,temp);
    sprintf(temp, "Content-Length: %d\r\n\r\n", 1);
    len += strlen(temp);
    buf = (char *)realloc(buf,sizeof(char)*len);
    strcat(buf,temp);

    buf = (char *)realloc(buf,sizeof(char)*(len+4));
    memcpy(&buf[len], "\r\n", 2);
    send(connect, buf, len, 0);
    printf("finish\n");
}

int main(void)
{
    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);
    pthread_t *thread = NULL;
    int clientFd, *temp = NULL;
    int opt = 1;
    char buffer[BUFFER_SIZE];
    char content[BUFFER_SIZE];

    int httpServerFd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(httpServerFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(PORT);
    server_sockaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(httpServerFd, (struct sockaddr*)&server_sockaddr, sizeof(server_sockaddr)) == -1) {
        printf("bind failed\n");
        return 0;
    }

    if (listen(httpServerFd, LISTEN_COUNT) == -1) {
        printf("listen failed!\n");
        return 0;
    }

    int client[LISTEN_COUNT] = {0};
    int clientCount = 0;
    fd_set readfd, tempfd;
    FD_ZERO(&readfd);
    FD_SET(httpServerFd, &readfd);
    struct timeval timeout;
    int maxfd = httpServerFd;

    while(1) {
        FD_ZERO(&tempfd);
        tempfd = readfd;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int ret = select(maxfd + 1, &tempfd, NULL, NULL, &timeout);
        if (ret < 0) {
            printf("select failed\n");
            return 0;
        }
        else if (ret == 0) {
            printf("timeout\n");
            continue;
        }

        //client have info
        for (int i = 0; i < clientCount; i++) {
            if (FD_ISSET(client[i], &tempfd)) {
                memset(buffer, 0, BUFFER_SIZE);
                int len = recv(clientFd, buffer, sizeof(buffer),0);
                if(len <= 0) {
                    printf("client = %d close\n", client[i]);
                    close(client[i]);
                    FD_CLR(client[i], &readfd);
                    client[i] = 0;
                    clientCount--;

                    continue;
                }

                printf("buf = %s\n", buffer);

                Request res = parseRequest(buffer);
                if (res.Expect != NULL) {
                    send(client[i], "HTTP/1.1 200 OK\r\n", 17, 0);
                    FILE *fp = fopen("file.txt", "wb");
                    if (fp == NULL) {
                        printf("fopen failed\n");
                    }
                    int totalLen = 0;
                    memset(content, 0, BUFFER_SIZE);
                    while (1) {
                        memset(buffer, 0, BUFFER_SIZE);
                        int len = recv(clientFd, buffer, sizeof(buffer), 0);
                        if (len > 0) {
                            fwrite(buffer, len, 1, fp);
                            strcat(content, buffer);
                            totalLen += len;

                            if (totalLen == atoi(res.content_Length))
                                break;
                        } else {
                            printf("recv failed\n");
                            break;
                        }
                    }
                    fclose(fp);
                }
                response(res, client[i]);
            }
        }

        //have client connect
        if (FD_ISSET(httpServerFd, &tempfd)) {
            clientFd = accept(httpServerFd, (struct sockaddr*)&client_addr, &length);
            if(clientFd == -1) {
                printf("accept failed\n");
                return 0;
            }
            client[clientCount++] = clientFd;
            FD_SET(clientFd, &readfd);
            if (clientFd > maxfd)
                maxfd = clientFd;
            printf("%d new client connect\n", clientFd);
        }
    }
}
