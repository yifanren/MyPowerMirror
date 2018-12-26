#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <ctype.h>
#include <string.h>

#include "include/curl/curl.h"
#include "include/util.h"

#define IP "192.168.11.111"
#define PORT 1239
#define NUMBER 9
#define BUFSIZE 128
#define FILEPATHSIZE 128
#define STRLINESIZE  1024
#define SENDINFOSIZE 10

struct input {
    FILE *in;
    size_t bytes_read; /* count up */
    CURL *curl;
};

typedef struct sendInfo{
    char path[32];
    char author[32];
    char age[32];
}send_t;

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
    struct input *i = userp;
    size_t retcode = fread(ptr, size, nmemb, i->in);
    i->bytes_read += retcode;
    return retcode;
}

void getSendInfo(send_t *sInfo, char* exInfo)
{
    printf("eXinfo is = %s\n", exInfo);
    if (strncmp(exInfo, "author:", strlen("author:")) == 0) {
        char *p = exInfo;
        while(*p != ':')
            p++;
        sprintf(sInfo->author, "author: %s", ++p);
    }
    if (strncmp(exInfo, "age:", strlen("age:")) == 0) {
        char *p = exInfo;
        while(*p != ':')
            p++;
        sprintf(sInfo->age, "age: %s", ++p);
    }
}

void* postUrl(send_t *sInfo)
{
    struct input indata;
    struct stat fileInfo;
    curl_off_t uploadsize;
    CURL *curl;
    char url[128] = {0};
    CURLcode res;
    char path[FILEPATHSIZE];

    strcpy(path, sInfo->path);
    struct curl_slist *slist = NULL;

    //get content type
    char *fileType = getFileType(path);
    char type[20];
    sprintf(type, "Content-type: %s", fileType);

    //set header
    slist = curl_slist_append(slist, type);
    slist = curl_slist_append(slist, "Expect: 100-continue");
    slist = curl_slist_append(slist, sInfo->author);
    slist = curl_slist_append(slist, sInfo->age);

    //get file size
    stat(path, &fileInfo);
    uploadsize = fileInfo.st_size;

    //url
    snprintf(url, sizeof(url), "http://%s:%d", IP, PORT);
    indata.in = fopen(path, "rb");
    indata.curl = curl_easy_init();
    if (indata.curl) {
        curl_easy_setopt(indata.curl, CURLOPT_URL, url);
        curl_easy_setopt(indata.curl, CURLOPT_POST, 1L);
        curl_easy_setopt(indata.curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(indata.curl, CURLOPT_READDATA, &indata);
        curl_easy_setopt(indata.curl, CURLOPT_POSTFIELDSIZE, uploadsize);
        curl_easy_setopt(indata.curl, CURLOPT_HTTPHEADER, slist);
        curl_easy_setopt(indata.curl, CURLOPT_TIMEOUT_MS, 5000);

        res = curl_easy_perform(indata.curl);
        printf("send reback res = %d\n", res);
        curl_easy_cleanup(indata.curl);
    }
    fclose(indata.in);
    return "seccess";
}

int main(void)
{
    int chooseNum;
    char str[STRLINESIZE];
    char strLine[STRLINESIZE][STRLINESIZE];
    int line = 0, num = 0;
    FILE *fp;
    struct stat buf;

    if ((fp = fopen("source.txt", "r")) == NULL) {
        printf("fopen failed\n");
        return -1;
    }

    while (fgets(str, STRLINESIZE, fp)) {
        strncpy(strLine[line], str, strlen(str) - 1);
        printf("num = %d, filepath:  %s\n", line, strLine[line]);
        if (line < STRLINESIZE)
            line++;
        else
            printf("file line is more than STRLINESIZE\n");
    }

    while (1) {
        printf("please input which one you want to send:");
        scanf("%d", &chooseNum);

        if (chooseNum >= line || chooseNum < 0) {
            printf("Not exist this file, please choose again\n");
            continue;
        }

        send_t *sInfo;
        sInfo = (send_t *)malloc(sizeof(send_t));
        printf("strline[chooseNum] = %s\n", strLine[chooseNum]);
        char temp[STRLINESIZE];
        strcpy(temp, strLine[chooseNum]);
        char* token = strtok(temp, " ");
        strcpy(sInfo->path, token);
        while(1) {
            token = strtok(NULL, " ");
            if (token == NULL)
                break;
            getSendInfo(sInfo, token);
        }

        if (stat(sInfo->path, &buf) == 0) {
            postUrl(sInfo);
            free(sInfo);
            sInfo = NULL;
        }
        else {
            printf("you choose the file is not exist, please check!\n");
            break;
        }
    }
    return 0;
}
