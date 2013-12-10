#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define MAX_MSG 1024
#define MAX_CONNEXIONS 10
#define PORT 44444

void* ecouteRequetes(void* arg);
void* gestionConnexion(void* arg);
void* envoyerPartition(void* arg);
void* importerFichier(void* arg);
void* obtenirPartition(void* arg);

int possedeFichier(char* cleFichier);
pthread_t lancerDaemon();
void inviteDeCommandes();
void initConnexion();


int main (int argc, char *argv[]) {
    //lancer daemon
    initConnexion();
    //pthread_t daemon = lancerDaemon();
    //inviteDeCommandes();
}

//THREAD -----------------------------------------------------------------

//-------DEAMON-------------------------------------

void* ecouteRequetes(void* arg) { //deamon
    int desClient;
    pthread_t th;
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("erreur socket\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in add;
    add.sin_family = AF_INET;
    add.sin_addr.s_addr = htonl(INADDR_ANY);
    add.sin_port = htons(PORT);


    if (bind(sock, (struct sockaddr *)&add, sizeof(add)) < 0) {
        perror("erreur bind");
        exit(EXIT_FAILURE);
    }

    if (listen(sock, MAX_CONNEXIONS) < 0) {
        perror("erreur listen");
    }

    struct sockaddr_in addCli;
    socklen_t sizeAddCli = sizeof(addCli);

    while (1) {
        desClient = accept((int)sock, (struct sockaddr *)&addCli, &sizeAddCli);
        if (desClient < 0) {
            perror("erreur accept");
        } else {
            /*paramsThread params;
            params.desCli = desClient;
            params.cleFichier = arg;
            pthread_create(&th, NULL, gestionConnexion, (void*)&params);*/
            pthread_create(&th, NULL, gestionConnexion, (void*)desClient);
        }
    }
    if (close(sock) < 0) {
		perror("erreur close");
		exit(EXIT_FAILURE);
	}
    pthread_exit(NULL);
}

void* gestionConnexion(void* arg) {
    //paramsThread* params = (paramsThread*)arg;
    int desCli = (int) arg;
    char msg[MAX_MSG];
    if (recv(desCli, msg, MAX_MSG, 0) < 0) {
        perror ("erreur recv");
    } else {
        char* numMsg = strtok(msg, " ");
        char* argm = strtok(NULL, " ");
        if (strcmp("302", numMsg) == 0) { // as tu une partition de ce fichier
            if(possedeFichier(argm)) { // cle fichier en parametre
                char rep[MAX_MSG] = "907";
                if(send(desCli, rep, strlen(rep), 0) < 0) {
                    perror("erreur send");
                    exit(EXIT_FAILURE);
                }
            } else {
                char rep[MAX_MSG] = "908";
                if(send(desCli, rep, strlen(rep), 0) < 0) {
                    perror("erreur send");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    /*printf("desc: %d\n", params->desCli);
    printf("cle: %s\n", params->cleFichier);*/

    pthread_exit(NULL);
}

//-----------DOWNLOAD--------------------------------

void* importerFichier(void* arg) {

    char* nomFichier = (char*)arg;
    pthread_t th;

    int sock = socket(PF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("erreur socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in add;
    add.sin_family = AF_INET;
    add.sin_addr.s_addr = inet_addr("10.5.8.23");
    add.sin_port = ntohs(PORT);

    if (connect(sock, (struct sockaddr*)&add, sizeof(add)) < 0) {
        perror("erreur connect");
        exit(EXIT_FAILURE);
    }

    char msg[MAX_MSG];
    strcat(msg, "300 ");
    strcat(msg, arg);

    if (send(sock, msg, strlen(msg), 0) < 0) { // demande liste des pairs ayant une partition du fichier arg
        perror("erreur envoi");
        exit(EXIT_FAILURE);
    }
    printf("message envoyé\n");

    char msgRecv[MAX_MSG];

    if (recv(sock, msgRecv, MAX_MSG, 0) < 0) {
        perror("erreur reception");
        exit(EXIT_FAILURE);
    }

    char* numMsg = strtok(msgRecv, " /");
    if (strcmp("301", numMsg) == 0) { // reception d un pair ayant ce fichier
        char* ip = strtok(NULL, " /");
        char* port = strtok(NULL, " /");

        struct sockaddr_in addPair;
        addPair.sin_family = AF_INET;
        add.sin_addr.s_addr = inet_addr(ip);
        add.sin_port = ntohs((uint16_t)port);

        pthread_create(&th, NULL, obtenirPartition, (void*)&addPair);

        void* partition;
        if (pthread_join(th, partition) != 0) {
            perror("erreur join");
        }
    }

    if (close(sock) < 0) {
		perror("erreur close");
		exit(EXIT_FAILURE);
	}
    pthread_exit(NULL);
}

void* obtenirPartition(void* arg) {
    struct sockaddr_in* addClient = (struct sockaddr_in*)arg;
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror( "erreur socket");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr*)&addClient, sizeof(addClient)) < 0) {
        perror("erreur connect");
        exit(EXIT_FAILURE);
    }

    char partition[MAX_MSG];

    if (recv(sock, partition, MAX_MSG, 0) < 0) {
        perror("erreur reception");
        exit(EXIT_FAILURE);
    }

	if (close(sock) < 0) {
		perror("erreur close");
		exit(EXIT_FAILURE);
	}

    pthread_exit((void*)partition);
}

//FONCTIONS ----------------------------------------------------

int possedeFichier(char* cleFichier) {
    char* fileNameData;
    strcpy(fileNameData, cleFichier);
    strcat(fileNameData, ".data");

    char* fileNamePart;
    strcpy(fileNamePart, cleFichier);
    strcat(fileNamePart, ".part");

    FILE *fd = fopen(fileNameData, "r");
    FILE *fp = fopen(fileNamePart, "r");

    if (fd != NULL && fp != NULL) {
        fclose(fd);
        fclose(fp);
        return 1;
    } else {
        return 0;
    }
}

pthread_t lancerDaemon() {
    pthread_t d;
    if (pthread_create(&d, NULL, ecouteRequetes, NULL) < 0) {
        perror("erreur creation daemon");
        exit(EXIT_FAILURE);
    }
    return d;
}


void inviteDeCommandes() {
    pthread_t th;
    char cmd[50];
    char* commande;
    char* arg;
    int exit = 0;
    while (exit != 1) {
        printf("Saisir commande:\n");

        scanf("%[a-z 0-9 ' ']", cmd);
        commande = strtok(cmd, " ");//getCommande(cmd);
        arg = strtok(NULL, " ");
        //printf("commande=%s\n", commande);
        //printf("arg=%s\n", arg);

        if (strcmp(commande, "export") == 0) {
            //printf("export ok\n");

        } else if (strcmp(commande, "import") == 0) {
            //printf("import ok\n");
            pthread_create(&th, NULL, importerFichier, (void*) arg);
        } else if (strcmp(commande, "exit") == 0) {
            exit = 1;
            printf("fin\n");
        } else {
            printf ("commande non reconnue\n");
        }
        cmd[0] = 0;
        //printf("CMD FIN: %s\n", cmd);
        while(getchar()!='\n');
    }
}

void initConnexion() {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("erreur socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in add;
    add.sin_family = AF_INET;
    add.sin_addr.s_addr = inet_addr("10.5.8.23");
    add.sin_port = ntohs(PORT);


    if (connect(sock, (struct sockaddr*)&add, sizeof(add)) < 0) {
        perror("erreur connect");
        exit(EXIT_FAILURE);
    }

    //TODO recuperer ip
    char msg[MAX_MSG] = "100 10.42.2.12/45123\0";

    if (send(sock, msg, strlen(msg), 0) < 0) {
        perror("erreur envoi");
        exit(EXIT_FAILURE);
    }

    printf("Message envoyé\n");

    /*char msgOk[MAX_MSG];

    if (recv(sock, msgOk, MAX_MSG, 0) < 0) {
        perror("erreur reception");
        exit(EXIT_FAILURE);
    }

    if (strcmp(msgOk, "901")) { //OK
        char msgCle[MAX_MSG];
        char* clePair = getClePair;
        if (clePair != NULL) {
            msgCle = "101";
        } else {
            //nouveau pair
            msgCle = "102";
        }


        if (send(sock, msgCle, strlen(msgCle), 0) < 0) {
            perror("erreur envoi");
            exit(EXIT_FAILURE);
        }
    }





    if (recv(sock, msgRecv, MAX_MSG, 0) < 0) {
        perror("erreur reception");
        exit(EXIT_FAILURE);
    }

    char* codeMsg = strtok(msgRecv, " ");
    if (strcmp(codeMsg, "901") == 0) {
        //envoyer clePair
    } else if (stcmp(codeMsg, ""))
       */
       if (close(sock) < 0) {
		perror("erreur close");
		exit(EXIT_FAILURE);
	}
}







