/* 
 * File:   main.c
 * Author: ulia22
 *
 * Created on 27 décembre 2013, 17:06
 */

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>

#define PORT 44444
/*
 * 
 */
int main(int argc, char** argv) {

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket()");
        exit(errno);
    }


    struct sockaddr_in sin = {0};

    sin.sin_addr.s_addr = htonl(INADDR_ANY); /* nous sommes un serveur, nous acceptons n'importe quelle adresse */

    sin.sin_family = AF_INET;

    sin.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr *) & sin, sizeof sin) == -1) {
        perror("bind()");
        exit(errno);
    }

    if (listen(sock, 5) == -1) {
        perror("listen()");
        exit(errno);
    }


    struct sockaddr_in csin = {0};
    int csock;

    int sinsize = sizeof csin;

    csock = accept(sock, (struct sockaddr *) & csin, &sinsize);

    if (csock == -1) {
        perror("accept()");
        exit(errno);
    }
    
    char buffer[100];
    recv(csock, buffer, 99, 0);
    printf(buffer);
    return (EXIT_SUCCESS);
}

