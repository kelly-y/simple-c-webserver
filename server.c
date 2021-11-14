#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>

#define PORT "8080"
#define BUF_SIZE 100000000

char buffer[BUF_SIZE + 1];

void sigHandler(int sig)
{   // 避免 zombie
    while( waitpid(-1, NULL, WNOHANG) > 0 );
}

const char *get_content_type(const char* path) {
    // 取得檔案類型
    const char *last_dot = strrchr(path, '.');
    if (last_dot) {
        if (strcmp(last_dot, ".css") == 0) return "text/css";
        if (strcmp(last_dot, ".csv") == 0) return "text/csv";
        if (strcmp(last_dot, ".gif") == 0) return "image/gif";
        if (strcmp(last_dot, ".htm") == 0) return "text/html";
        if (strcmp(last_dot, ".html") == 0) return "text/html";
        if (strcmp(last_dot, ".ico") == 0) return "image/x-icon";
        if (strcmp(last_dot, ".jpeg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".jpg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".js") == 0) return "application/javascript";
        if (strcmp(last_dot, ".json") == 0) return "application/json";
        if (strcmp(last_dot, ".png") == 0) return "image/png";
        if (strcmp(last_dot, ".pdf") == 0) return "application/pdf";
        if (strcmp(last_dot, ". svg") == 0) return "image/svg+xml";
        if (strcmp(last_dot, ".txt") == 0) return "text/plain";
    }

    return "application/octet-stream";
}

void get(int newfd, char* path)
{
    // 打開指定檔案
    FILE *fp = fopen(path, "rb");
    fseek(fp, 0L, SEEK_END);
    size_t cl = ftell(fp);
    rewind(fp);
    const char *ct = get_content_type(path);

    char buffer_get[1024];

    // 發送 200 ok
    sprintf(buffer_get, "HTTP/1.1 200 OK\r\n");
    send(newfd, buffer_get, strlen(buffer_get), 0);

    sprintf(buffer_get, "Connection: close\r\n");
    send(newfd, buffer_get, strlen(buffer_get), 0);

    sprintf(buffer_get, "Content-Length: %lu\r\n", cl);
    send(newfd, buffer_get, strlen(buffer_get), 0);

    sprintf(buffer_get, "Content-Type: %s\r\n", ct);
    send(newfd, buffer_get, strlen(buffer_get), 0);

    sprintf(buffer_get, "\r\n");
    send(newfd, buffer_get, strlen(buffer_get), 0);

    // 發送檔案內容
    int r = fread(buffer_get, 1, 1024, fp);
    while (r) {
        send(newfd, buffer_get, r, 0);
        r = fread(buffer_get, 1, 1024, fp);
    }

    fclose(fp);

    return;
}

void post(int newfd)
{
    // 找到各自標題 (key)
    char *boundary = strstr(buffer, "boundary");
    char *fileName = strstr(buffer, "filename");
    char *contType = strstr(fileName, "Content-Type: ") + 1;

    // 處理檔名
    char fname[51];
    fileName += 10;
    char *fileNameEnd = strchr(fileName, '"');
    strncpy(fname, fileName, fileNameEnd-fileName);
    fname[fileNameEnd-fileName] = '\0';

    // 處理內容
    boundary += 9;  // 跳過"boundary="
    char *nxtLn = strstr(boundary, "\r\n");
    char bondCont[81];
    strncpy(bondCont, boundary, nxtLn-boundary);
    bondCont[nxtLn-boundary] = '\0';
    char *content = strstr(contType, "\r\n") + 4;
    char *contEnd = strstr(content, bondCont) - 4;
    *contEnd = '\0';

    // 存檔
    char path[61] = "./";
    strcat(path, fname);
    FILE *fp = fopen(path, "wb");
    size_t rt = fwrite(content, sizeof(char), contEnd-content, fp);
    fclose(fp);
    chmod(path, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);

    // 回到原頁面
    get(newfd, "index.html");

    return;
}

void childProcess(int newfd)
{
    // 接收 request 內容
    int length = recv(newfd, buffer, BUF_SIZE, 0);
    if(length==0 || length==-1) // 網路連線異常
        exit(3);

    // 把字串結尾補 0
    if(length>0 && length<BUF_SIZE)
        buffer[length] == 0;
    else
        buffer[0] = 0;

    // 分辨 get & post
    if( strncmp(buffer, "GET", 3)==0 || strncmp(buffer, "get", 3)==0)
    {
        char *startPath = buffer + 5;
        char *endPath = strstr(startPath, " ");
        *endPath = '\0';
        get(newfd, startPath);
    }
    else if(strncmp(buffer, "POST", 4)==0 || strncmp(buffer, "post", 4)==0)
    {
        post(newfd);
    }
    else
        exit(3);

    return;
}

int main(int argc, char *argv[])
{
    // 取得 addr info
    struct addrinfo info, *addr;
    memset(&info, 0, sizeof(info));     // 清空 info
    info.ai_family = AF_INET;           // IPv4
    info.ai_socktype = SOCK_STREAM;     // TCP
    info.ai_flags = AI_PASSIVE;         // 填 IP
    int rtAddr = getaddrinfo(NULL, PORT, &info, &addr);
    if(rtAddr != 0)
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rtAddr));

    // 建立一個 socket
    int sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if(sockfd == -1)
        perror("server socket");

    // 允許重新使用 port
    int yes = 1;
    if( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 )
        perror("setsockopt");

    // Bind
    int rtBd = bind(sockfd, addr->ai_addr, addr->ai_addrlen);
    if(rtBd == -1)
        perror("server bind");

    // Listen
    int rtLsn = listen(sockfd, 10);
    if(rtLsn == -1)
        perror("listen");

    // Signal Handler
    signal(SIGCHLD, sigHandler);

    // 開始接收 Request
    int newfd, pid;
    socklen_t sockLen;
    struct sockaddr client;
    while(1)
    {
        // Accept
        sockLen = sizeof(client);
        newfd = accept(sockfd, &client, &sockLen);
        if(newfd == -1)
            perror("accept");

        pid = fork();
        if(pid < 0)
            perror("fork failed");
        else if (pid == 0)  // child
        {
            close(sockfd);
            childProcess(newfd);
            close(newfd);
            exit(0);

        }
        else                // parent
            close(newfd);
    }
    return 0;
}