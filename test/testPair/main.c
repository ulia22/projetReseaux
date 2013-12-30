/* 
 * File:   main.c
 * Author: ulia22
 *
 * Created on 27 décembre 2013, 16:59
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

    struct hostent *hostinfo = NULL;
    struct sockaddr_in sin = {0}; /* initialise la structure avec des 0 */
    const char *hostname = "localhost";

    hostinfo = gethostbyname(hostname); /* on récupère les informations de l'hôte auquel on veut se connecter */
    if (hostinfo == NULL) /* l'hôte n'existe pas */ {
        fprintf(stderr, "Unknown host %s.\n", hostname);
        exit(EXIT_FAILURE);
    }

    sin.sin_addr = *(struct in_addr *) hostinfo->h_addr; /* l'adresse se trouve dans le champ h_addr de la structure hostinfo */
    sin.sin_port = htons(PORT); /* on utilise htons pour le port */
    sin.sin_family = AF_INET;

    if (connect(sock, (struct sockaddr *) & sin, sizeof (struct sockaddr)) == -1) {
        perror("connect()");
        exit(errno);
    }

    char msg[100] = "100 127.0.0.1/44444";
    send(sock, msg, strlen(msg), 0);
    
    memset(msg, 0, strlen(msg));
    recv(sock, msg, 100, 0);
    printf("Valeur ok (901):%s\n", msg);
    
    memset(msg, 0, strlen(msg));
    strcat(msg, "102");
    send(sock, msg, strlen(msg), 0);
    
    memset(msg, 0, strlen(msg));
    recv(sock, msg, 100, 0);
    printf("Message recu (ok  ): %s\n", msg);
    
    memset(msg, 0, strlen(msg));
    strcat(msg, "901");
    send(sock, msg, strlen(msg), 0);
    while(1){
        
         
    }

    return (EXIT_SUCCESS);
}

