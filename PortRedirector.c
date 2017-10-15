#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <arpa/inet.h>

#define BUF_SIZE 4096
#define QUEUE_SIZE 100

short sourceport = 0;
short targetport = 0;

pthread_mutex_t conp_mutex;

void dealonereq(void *arg);
int connectserver();

int main(int argc, char **argv) {
    if(argc != 3) {
        printf("The count of port number error!\n");
        return -1;
    }

    sourceport = (short) atoi(argv[1]);
    targetport = (short) atoi(argv[2]);
    if(sourceport == 0 || targetport == 0) {
        printf("Invalid port number, try again!\n");
        printf("Usage: %s sourceport targetport\n", argv[0]);
        return -1;
    }

    struct sockaddr_in client_addr, target_addr;
    socklen_t sin_size = sizeof(struct sockaddr_in);
    int sockfd, accept_sockfd, on = 1;
    pthread_t Clitid;

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0) {
        printf("Socket failed...Abort...\n");
        return -1;
    }

    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    target_addr.sin_port = htons(sourceport);

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));

    if(bind(sockfd, (struct sockaddr *) &target_addr, sizeof(target_addr)) < 0) {
        printf("Bind failed...Abort...\n");
        return -1;
    }
    if(listen(sockfd, QUEUE_SIZE) < 0) {
        printf("Listen failed...Abort...\n");
        return -1;
    }
    while(1) {
        accept_sockfd = accept(sockfd, (struct sockaddr *) &client_addr, &sin_size);
        if(accept_sockfd <= 0) {
            printf("accept failed\n");
            continue;
        }
        printf("Received a request from %s: %u\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        pthread_create(&Clitid, NULL, (void *) dealonereq, (void *) accept_sockfd);
    }
    return 0;
}

void dealonereq(void *arg) {
    char buf[BUF_SIZE];
    char recvbuf[BUF_SIZE];
    int bytes;
    int accept_sockfd;
    int remotesocket;

    accept_sockfd = (int) arg;
    pthread_detach(pthread_self());
    bzero(buf, BUF_SIZE);
    bzero(recvbuf, BUF_SIZE);
    bytes = read(accept_sockfd, buf, BUF_SIZE);
    if(bytes <= 0) {
        printf("Cannot read from accept_sockfd\n");
        close(accept_sockfd);
        return;
    }

    remotesocket = connectserver();
    if(remotesocket == -1) {
        printf("Cannot connect to targetport\n");
        close(accept_sockfd);
        return;
    }
    send(remotesocket, buf, bytes, 0);
    while(1) {
        int readSizeOnce = 0;
        readSizeOnce = read(remotesocket, recvbuf, BUF_SIZE);

        if(readSizeOnce <= 0) {
            break;
        }
        send(accept_sockfd, recvbuf, readSizeOnce, 0);
    }
    close(remotesocket);
    close(accept_sockfd);
}

int connectserver() {
    int cnt_stat;
    struct sockaddr_in server_addr;
    int remotesocket;
    remotesocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(remotesocket < 0) {
        printf("Cannot create socket!\n");
        return -1;
    }
    memset(&server_addr, 0 ,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(targetport);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    cnt_stat = connect(remotesocket, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if(cnt_stat < 0) {
        printf("remote connect failed\n");
        close(remotesocket);
        return -1;
    }
    else
        printf("connected remote server--------------->%s: %u.\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    return remotesocket;
}