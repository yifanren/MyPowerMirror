#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 1233
#define BUFSIZE 128

int main(void)
{
    int clifd;
    struct sockaddr_in remote_addr;
    char buf[BUFSIZE];
    int ret;

    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    remote_addr.sin_port = htons(PORT);

    if ((clifd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        perror("client socket failed\n");
        exit(1);
    }

    ret = connect(clifd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));
    if (ret < 0) {
        perror("connect failed\n");
        close(clifd);
        exit(1);
    }

    strcpy(buf, "ping"); 
    ret = write(clifd, buf, strlen(buf));
    printf("send to server message:ping\n");
    if (ret == -1) {
        perror("client send failed\n");
        close(clifd);
        exit(1);
    }

    while(1) {
        memset(buf, 0, BUFSIZE);
        ret = read(clifd, buf, BUFSIZE);

        printf("client recv message :%s\n",buf);

    }
    close(clifd);
}
