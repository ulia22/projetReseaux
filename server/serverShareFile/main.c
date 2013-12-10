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
#include <regex.h>

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
    
    printf("Server running...\n");
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
    inet_aton("10.5.8.23", &server_addr.sin_addr.s_addr);
    client_addr_size = sizeof(struct sockaddr_in);
    
    //Lie la socket server à une adresse ip et un port.
    if(bind(sdServerAccept, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {perror("Erreur Bind.\n"); 
    exit(EXIT_FAILURE);}
    
    //Ecoute sur le port désigné.
    if(listen(sdServerAccept, LISTEN_LIMIT) == -1){
        perror("Erreur listen.\n");
        exit(EXIT_FAILURE);}
    
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
        printf("Message : %s\n", buffer);
        if(strncmp(buffer, "100", 3) == 0){//Nouveau client
            //On lit les arguments du message en entier.
            memset(buffer, 0, MSG_BUFFER_SIZE);       
            recv(sdClient, buffer, 15+5+3, 0);/*longueur addrIp + longueur port + 1 '/' et 1 espace + 1 caractère \0.*/
            if(initClientConnect(sdClient, buffer) != 0){
                printf("Erreur initClientConnect.\n");
                exit(EXIT_FAILURE);
            }
        }else if(strncmp(buffer, "200", 3) == 0){//Partage d'un fichier
            
        }else if(strncmp(buffer, "300", 3) == 0){//DL d'un fichier
            
        }else if(strncmp(buffer, "400", 3) == 0){//Mise a jour meta-data
            
        }
    }
    shutdown(sdClient, 2);
    pthread_exit((void*)id_t);
}

int initClientConnect (int sdClient, char* msg){
    
    printf("%s\n", msg);
    //Preparation de l'extraction de l'addresse
    char addrIp[16] = {'\0'};
    char port[6] = {'\0'};
    if(extractIpPort(msg, addrIp, port) == -1){
        perror("Erreur extractIpPort.\n");
        exit(EXIT_FAILURE);
    }
     return 0;
}



/**
 * Permet d'extraire l'addresse ip et le port d'une chaine si ils sont sous la forme " ip/port".
 * @param msg contenant l'addrIp/Port
 * @param port chaine qui contiendra le port
 * @param ip chaine qui contiendra l'addrIp
 * @return 0 si reussi -1 sinon
 */
int extractIpPort(char* msg, char* port, char* ip){
    regex_t preg;
    char str_regex[] = "^ *([[:digit:]\\.]+)\\([:digit:]{1,6})"; 
    int err = 0;
    
    err = regcomp (&preg, str_regex, REG_EXTENDED);
    if(err != 0){
        fprintf(stderr, "Erreur compilation regex addr/port.\n");
        exit(EXIT_FAILURE);
    }
    
    int match;
    size_t nmatch = 0;
    regmatch_t *pmatch = NULL;
 
    nmatch = preg.re_nsub;
    pmatch = malloc (sizeof (*pmatch) * nmatch);
    
    
    //Lecture du message, on recherche l'addrIP et le port.
    
    // Recherche d'occurences.
     match = regexec (&preg, msg, nmatch, pmatch, 0);
     regfree (&preg);
   
     //Si la chaine est reconnue.
     if (match == 0)
         {
            //Extraction de l'adresse IP.
            int startIp = pmatch[0].rm_so;
            int endIp = pmatch[0].rm_eo;
            size_t sizeIp = endIp - startIp;

            if (ip)
            {
               strncpy (ip, &msg[startIp], sizeIp);
               ip[sizeIp] = '\0';
            }
            
            //Extraction du port.
            int startPort = pmatch[1].rm_so;
            int endPort = pmatch[1].rm_eo;
            size_t sizePort = endPort - startPort;
            
            if (port)
            {
               strncpy (port, &msg[startPort], sizePort);
               port[sizePort] = '\0';
            }
            printf ("Nouveau pair à l'adresse IP : %s:%s\n", ip,port);
         }else if(match == REG_NOMATCH){
                fprintf(stderr, "Pas de correspondance avec la regex.\n");
                return -1;
         }else{
            char *text;
            size_t size;

            size = regerror (err, &preg, NULL, 0);
            text = malloc (sizeof (*text) * size);
            if (text)
            {
/* (8) */
               regerror (err, &preg, text, size);
               fprintf (stderr, "%s\n", text);
               free (text);
            }
            else
            {
               fprintf (stderr, "Memoire insuffisante\n");
               exit (EXIT_FAILURE);
            }
         }
     return 0;
}