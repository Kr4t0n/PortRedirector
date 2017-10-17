#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define END(message) perror(message); exit(1);
#define BUF_SIZE 4096
#define QUEUE_SIZE 100

short sourceport = 0;
short targetport = 0;

void dealonereq(int client_socket);
void communicate(int src, int dst);

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

    signal(SIGCHLD,  SIG_IGN);
    struct sockaddr_in client_address, server_address;
    socklen_t sin_size = sizeof(struct sockaddr_in);
    int listen_socket, client_socket;
    pid_t up_pid;

    listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(listen_socket == -1) {
        END("Socket failed...Abort...");
    }

    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(sourceport);

    if(bind(listen_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
        END("Bind failed");
    }
    if(listen(listen_socket, QUEUE_SIZE) == -1) {
        END("Listen failed");
    }
    while(1) {
        client_socket = accept(listen_socket, NULL, NULL);

        if(client_socket == -1) {
            END("accept failed");
        }
        
        up_pid = fork();

        if(up_pid == -1) {
            END("fork failed");
        }

        if(up_pid == 0) {
            dealonereq(client_socket);
            exit(1);
        }

        close(client_socket);
    }

    return 0;
}

void dealonereq(int client_socket) {
    int forward_socket;
    struct sockaddr_in forward_address;
    pid_t down_pid;

    forward_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(forward_socket == -1) {
        END("forward socket failed");
    }

    bzero((char *) &forward_address, sizeof(forward_address));
    forward_address.sin_family = AF_INET;
    forward_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    forward_address.sin_port = htons(targetport);

    if(connect(forward_socket, (struct sockaddr *) &forward_address, sizeof(forward_address)) == -1) {
        END("forward connect failed");
    }

    down_pid = fork();

    if(down_pid == -1) {
        END("fork failed");
    }

    if(down_pid == 0){
        communicate(forward_socket, client_socket);
    }
    else {
        communicate(client_socket, forward_socket);
    }
}

void communicate(int src, int dst) {
    char buf[BUF_SIZE];
    int r, i, j;

    r = read(src, buf, BUF_SIZE);
    while(r > 0) {
        i = 0;
        while(i < r) {
            j = write(dst, buf + i, r - i);
            if (j == -1) {
                END("write failed");
            }
            i += j;
        }
        r = read(src, buf, BUF_SIZE);
    }
    if (r == -1) {
        END("read failed");
    }

    shutdown(src, SHUT_RD);
    shutdown(dst, SHUT_WR);
    close(src);
    close(dst);
    exit(0);  
}