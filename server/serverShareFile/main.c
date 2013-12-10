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
#include <arpa/inet.h>
#include <string.h>

#include "clientData.h"

#define MSG_BUFFER_SIZE 1024

//Port de'ecoute du server.
#define PORT 44444

//Nombre d'ecoute en attente maximum.
#define LISTEN_LIMIT 10

void *manageClient(void* par);
/*
 * 
 */
int main(int argc, char** argv) {
    
    printf("This is a test.\n");
    //Definition des variables pour acceder aux threads
    pthread_t client_t;
    //Definition des variables pour ouvrir les connections.
    int sdServerAccept, sdClient;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    
    //Obtenir descripteur de socket pour le server.
    sdServerAccept = socket(AF_INET, SOCK_STREAM, 0);
    if(sdServerAccept == -1){
        perror("Erreur création de la socket server.\n");
        exit(EXIT_FAILURE);}
    
    //Instanciation de la structure pour lier la socket à l'adresse et au port du server.
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_aton("127.0.0.1", &server_addr.sin_addr.s_addr);
    client_addr_size = sizeof(struct sockaddr_in);
    
    //Lie la socket server à une adresse ip et un port.
    if(bind(sdServerAccept, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {perror("Erreur Bind.\n"); 
    exit(EXIT_FAILURE);}
    
    //Ecoute sur le port désigné.
    if(listen(sdServerAccept, LISTEN_LIMIT) == -1){
        perror("Erreur listen.\n");
        exit(EXIT_FAILURE);}
    printf("Test2\n");
    //Accepte les connections entrante et laisse la gestion à un nouveau thread.
    while(1){
        sdClient = accept(sdServerAccept, (struct sockaddr *)&client_addr, &client_addr_size);
        if(sdClient == -1){
            perror("Erreur accept.\n"); 
            exit(EXIT_FAILURE);}
        
        //Crée le thread qui va s'occuper du client.
        if(pthread_create(&client_t, NULL, manageClient, (void*) &sdClient) != 0){
            perror("Erreur lancement d'un thread.\n");
            exit(EXIT_FAILURE);
        }
        printf("Test1\n");
    }
    return (EXIT_SUCCESS);
}



/*
 * Permet de gérer une connection sur le socket sdClient dans un thread.
 */
void *manageClient(void* par){
    pthread_t id_t = pthread_self(); 
    //Socket client
    int sdClient = *((int*)par);
    char buffer[MSG_BUFFER_SIZE] = {'\0'};
    
    while(strncmp(buffer, "909", 3) != 0){
        
        memset(buffer, 0, MSG_BUFFER_SIZE);       
	recv(sdClient, buffer, 3, 0);
        printf("Bonjour code message %s \n.", buffer);
        
        if(strncmp(buffer, "100", 3) == 0){//Nouveau client
            
        }else if(strncmp(buffer, "200", 3) == 0){//Partage d'un fichier
            
        }else if(strncmp(buffer, "300", 3) == 0){//DL d'un fichier
            
        }else if(strncmp(buffer, "400", 3) == 0){//Mise a jour meta-data
            
        }
    }
    shutdown(sdClient, 2);
    pthread_exit((void*)id_t);
}

