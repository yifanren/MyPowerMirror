#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define PORT       1233
#define BUFSIZE    128
#define MAXCONNECT 1024

int main(void) {
    
    int serverfd,clientfd;
    struct sockaddr_in serverAdd;
    struct sockaddr_in clientAdd;
    socklen_t addrlen;
    char buf[BUFSIZE];
    int clientSock[MAXCONNECT];
    int len;

    if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("server socket failed\n");
        exit(1);
    }

    memset(&serverAdd, 0, sizeof(serverAdd));
    serverAdd.sin_family = AF_INET;
    serverAdd.sin_addr.s_addr = INADDR_ANY;
    serverAdd.sin_port = htons(PORT);

    if (bind(serverfd, (struct sockaddr *)&serverAdd, sizeof(serverAdd)) == -1) {
        perror("server bind failed\n");
        close(serverfd);
        exit(1);
    }

    if (listen(serverfd, MAXCONNECT) == -1) {
        perror("server listen failed\n");
        close(serverfd);
        exit(1);
    } 
    printf("listen ok \n");

    fd_set readfd;
    int maxfd = serverfd;
    int connectCount = 0;
    addrlen = sizeof(clientAdd);
    
    while(1) {
        FD_ZERO(&readfd);
        FD_SET(serverfd, &readfd);//添加服务器描述符
        for(int i = 0; i < connectCount; i++) {
            if (clientSock[i] != 0)
                FD_SET(clientSock[i], &readfd);
        }

        int fds = select(maxfd + 1, &readfd, NULL, NULL, NULL);
        if (fds < 0) {
            perror("select failed\n");
            close(clientfd);
            close(serverfd);
            exit(1);
        } else if (fds == 0) {
            continue;
        }

        printf("xunhuan jici\n");
        //have info
        for ( int i = 0; i < connectCount; i++) {
            if (FD_ISSET(clientSock[i], &readfd)) {
                //client send message
                printf("i = %d, connectCount = %d, clientSock[i] = %d\n", i, connectCount, clientSock[i]);
                memset(buf, 0, BUFSIZE);
                len = recv(clientSock[i], buf, BUFSIZE, 0);
                if (len > 0) {
                    printf("server recv message :%s\n", buf);
                    memset(buf, 0, BUFSIZE);
                    strcpy(buf, "pong");
                    send(clientfd, buf, strlen(buf), 0);
                    FD_CLR(clientSock[i], &readfd);
                }

                printf("len = %d\n", len);
                if (len == 0) {
                    printf("client %d close\n", clientSock[i]);
                    clientSock[i] = 0;
                    connectCount--;
                    maxfd--;
                    close(clientSock[i]);
                }
            }
        }
        
        if (FD_ISSET(serverfd, &readfd)) {
                //new connect
            if ((clientfd = accept(serverfd, (struct sockaddr *)&clientAdd, &addrlen)) == -1) {
                perror("server accept failed\n");
                close(serverfd);
                exit(1);
            }

            clientSock[connectCount++] = clientfd;
            if (clientfd > maxfd)
                maxfd = clientfd;

            printf(" %d new client connect\n", clientfd);
        }
    }
   
    for (int  i = 0; i < connectCount; i++ )
        close(clientSock[i]);
    close(serverfd);
}
