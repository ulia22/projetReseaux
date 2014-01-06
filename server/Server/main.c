/* 
 * File:   main.c
 * Author: cbarbaste
 *
 * Created on 3 décembre 2013, 15:12
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

#include "fileData.h"
#include "pair.h"

#define MSG_BUFFER_SIZE 1024

//Port/AddrIP d'ecoute du server.
#define PORT 44444
#define ADDR_SERVER "127.0.0.1"

//Nombre d'ecoute en attente maximum.
#define LISTEN_LIMIT 10

//Fonctions.
int init(void);
void *manageClient(void* par[]);
int extractIpPort(char* msg, char* port, char* ip);
int initListeningSocket(void);

extern pair* ptrPair;
/*
 * 
 */
int main(int argc, char** argv) {
    printf("Server is running...\n");
    init();    
    ///Test seg fault
    pair* p = NULL;
    addPair("5", "1.1.1.1", 4555);
    p = findPair(5);
    printf("Clé: %d\nAddr: %s\nPort: %d\nNext: %p\nPrev: %p\n", p->clePair, p->addrPair, p->portPair, p->next, p->prev);
    
    addPair("4", "2.2.2.2", 5555);
    p = findPair(4);
    printf("Clé: %d\nAddr: %s\nPort: %d\nNext: %p\nPrev: %p\n", p->clePair, p->addrPair, p->portPair, p->next, p->prev);
    
    addPair("7", "2.2.2.2", 5555);
    p = findPair(7);
    printf("Clé: %d\nAddr: %s\nPort: %d\nNext: %p\nPrev: %p\n", p->clePair, p->addrPair, p->portPair, p->next, p->prev);
    
    addPair("6", "2.2.2.2", 5555);
    p = findPair(6);
    printf("Clé: %d\nAddr: %s\nPort: %d\nNext: %p\nPrev: %p\n", p->clePair, p->addrPair, p->portPair, p->next, p->prev);
    
    addPair("3", "2.2.2.2", 5555);
    p = findPair(3);
    printf("Clé: %d\nAddr: %s\nPort: %d\nNext: %p\nPrev: %p\n", p->clePair, p->addrPair, p->portPair, p->next, p->prev);
    ///Fin test////
    
    initFileData();
    //Definition des variables pour ouvrir les connections.
    int sdServerAccept;
    //Initialisation de la socket d'ecoute du server.
    sdServerAccept = initListeningSocket();
    //Accepte les connections entrante et laisse la gestion à un nouveau thread.
    while (1) {
        acceptClient(sdServerAccept);
    }
    return (EXIT_SUCCESS);
}

/*
 * Permet de gérer une connection sur le socket sdClient dans un thread.
 */
void *manageClient(void* par[]) {
    pthread_t id_t = pthread_self();
    //Socket client
    int sdClient = ((acceptThreadArg*) par)->sock;
    struct sockaddr_in client_addr = ((acceptThreadArg*) par)->pair;
    char buffer[MSG_BUFFER_SIZE];
    int ret = 0;
    
    printf("sock: %d\n", sdClient);
    while (strncmp(buffer, "909", 3) != 0) {

        //Lecture du code du message.
        memset(buffer, 0, MSG_BUFFER_SIZE);
        ret = recv(sdClient, buffer, 3, 0);
        if(ret <= 0){
            strcat(buffer, "909");
        }
        
        printf("Buffer: %s\n", buffer);
        if (strncmp(buffer, "100", 3) == 0) {//Nouveau client
            if (initClientConnect(sdClient, client_addr) != 0) {
                printf("Erreur initClientConnect.\n");
                exit(EXIT_FAILURE);
            }
        } else if (strncmp(buffer, "200", 3) == 0) {//Partage d'un fichier
           if(shareFile(sdClient, client_addr) != 0){
            printf("Erreur partage fichier.\n");
            exit(EXIT_FAILURE);
           }
        } else if (strncmp(buffer, "300", 3) == 0) {//DL d'un fichier
            if(dlFile(sdClient, client_addr) != 0){
                printf("Erreur DL fichier.\n");
                exit(EXIT_FAILURE);
            }
        } else if (strncmp(buffer, "400", 3) == 0) {//Mise a jour meta-data

        }
    }
    //closesocket(sdClient);
    close(sdClient);
    pthread_exit((void*) id_t);
}

/**
 * Init des structures des données globales, plus tard prise en charge des argv et argc.
 * @return 0 si tout va bien sinon -1.
 */
int init(void) {
    ptrPair = NULL;
    return 0;
}

/**
 * Initialise la socket d'ecoute du server.
 * @param serverSocket pointeur sur la socket d'ecoute du server.
 * @return la socket d'ecoute du server.
 */
int initListeningSocket(void) {
    struct sockaddr_in server_addr = {0};
    int serverSocket = 0;

    //Obtenir descripteur de socket pour le server.
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Erreur création de la socket server.\n");
        exit(EXIT_FAILURE);
    }

    //Instanciation de la structure pour lier la socket à l'adresse et au port du server.
    memset(&server_addr, 0, sizeof (struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    //inet_aton(ADDR_SERVER, &server_addr.sin_addr.s_addr);

    //Lie la socket server à une adresse ip et un port.
    if (bind(serverSocket, (struct sockaddr*) &server_addr, sizeof (server_addr)) == -1) {
        perror("Erreur Bind.\n");
        exit(EXIT_FAILURE);
    }

    //Ecoute sur le port désigné.
    if (listen(serverSocket, LISTEN_LIMIT) == -1) {
        perror("Erreur listen.\n");
        exit(EXIT_FAILURE);
    }

    return serverSocket;
}

/**
 * Accepte les client qui veulent se connecter sur la socket d'ecoute du server.
 * Créé un thread pour gérer les nouveaux clients.
 * @return descripteur de socket du client.
 */
int acceptClient(int serverSocket) {
    struct sockaddr_in client_addr = {0};
    int size = sizeof client_addr;
    int sockClient = 0;
    pthread_t client_t;
    acceptThreadArg arg;

    //Accepte le client.
    if ((sockClient = accept(serverSocket, (struct sockaddr *) &client_addr, &size)) == -1) {
        perror("Erreur accept.\n");
        exit(EXIT_FAILURE);
    }

    //Créé le thread qui va s'occuper du client.
    arg.sock = sockClient;
    arg.pair = client_addr;
    if (pthread_create(&client_t, NULL, manageClient, (void*) &arg) != 0) {
        perror("Erreur lancement d'un thread.\n");
        exit(EXIT_FAILURE);
    }
    return sockClient;
}