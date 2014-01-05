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

#include "pair.h"

#define MAX_MSG 1024
#define MAX_CONNEXIONS 10
#define PORT 44444
#define TAILLE_CLE_PAIR 10

void* ecouteRequetes(void* arg);
void* gestionConnexion(void* arg);
void* envoyerPartition(void* arg);
void* importerFichier(void* arg);
void* obtenirPartition(void* arg);

char* getFichier (char* cle);
ListePair* getListePairFichier(char* metadata);
void enregistrerFichier(char* nom, char* contenu);
int possedeFichier(char* cleFichier);
pthread_t lancerDaemon();
void inviteDeCommandes();
void initConnexion();
char* getClePair();
int enregistrerNouvelleCle(char* cle);



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
        } else if (strcmp("500", numMsg) == 0) { // demande d'un fichier a envoyer pour un pair
            if(possedeFichier(argm)) {
                char partition[MAX_MSG] = "501 ";
                strcat(partition, getFichier(argm));
                if(send(desCli, partition, strlen(partition), 0) < 0) { //envoi du message 501 contenudufichier
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


    char msgRecv[MAX_MSG];

    if (recv(sock, msgRecv, MAX_MSG, 0) < 0) {
        perror("erreur reception");
        exit(EXIT_FAILURE);
    }

    char* numMsg = strtok(msgRecv, " /");
    if (strcmp("301", numMsg) == 0) { // reception de la liste des pairs ayant une partition
        /*char* ip = strtok(NULL, " /");
        char* port = strtok(NULL, " /");*/
        char* metadata = strtok(msgRecv, " ");
        ListePair *pairs = getListePairFichier(metadata);


        Pair *p = pairs->prem;
        char* part;
        char* ip;
        char* port;
        int recu = 0;
        //int part_traites[] = {0}
        //TODO gerer les partitions : recuperer le nombre de partitions
        while (p->suiv != NULL && !recu) {
            part = strtok(p->data, " ");
            ip = strtok(NULL, " ");
            port = strtok(NULL, " ");

            struct sockaddr_in addPair;
            addPair.sin_family = AF_INET;
            add.sin_addr.s_addr = inet_addr(ip);
            add.sin_port = ntohs((uint16_t)port);

            ParamPartition *params;
            params->add = addPair;
            params->cleFichier = nomFichier; // TODO : recuperer la clé du fichier plutot que le nom (faire en sorte que le serveur l'envoi ?


            pthread_create(&th, NULL, obtenirPartition, (void*)&params);

            void* partition;
            if (pthread_join(th, partition) != 0) {
                perror("erreur join");
                exit(EXIT_FAILURE);
            }

            if((char*)partition != "") {
                enregistrerFichier(nomFichier, partition);
                recu = 1;
            }
            p = p->suiv;
        }
        if (!recu) { // pas de fichier telechargé
            printf("Impossible de telecharger le fichier\n");
        }
    }

    if (close(sock) < 0) {
		perror("erreur close");
		exit(EXIT_FAILURE);
	}
    pthread_exit(NULL);
}

void* obtenirPartition(void* arg) {
    ParamPartition *params = (ParamPartition*)arg;
    //struct sockaddr_in* addClient = (struct sockaddr_in*)arg;
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror( "erreur socket");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr*)&params->add, sizeof(&params->add)) < 0) {
        perror("erreur connect");
        exit(EXIT_FAILURE);
    }

    char msgDemandeFichier[MAX_MSG];
    strcat(msgDemandeFichier, "500 ");
    strcat(msgDemandeFichier, params->cleFichier);

    if (send(sock, msgDemandeFichier, strlen(msgDemandeFichier), 0) < 0) { //demande telechargement fichier
        perror("erreur envoi");
        exit(EXIT_FAILURE);
    }


    char msgPartition[MAX_MSG];

    if (recv(sock, msgPartition, MAX_MSG, 0) < 0) { // reception de la partition
        perror("erreur reception");
        exit(EXIT_FAILURE);
    }

    char* partition = "";

    char* codeMsg = strtok(partition, " ");
    if (strcmp(codeMsg, "501") == 0) { //
         partition = strtok(NULL, " ");
    }

	if (close(sock) < 0) {
		perror("erreur close");
		exit(EXIT_FAILURE);
	}

    pthread_exit((void*)partition);
}

//---------------UPLOAD-------------------------------------

/*void* exporterFichier(void* path) {
    char* nom = (char*)path;
    char contenu[MAX_MSG];
    contenu = ouvrirFichier(path); //TODO fonction a implementer
    if (contenu != "") {

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

        char msgDemandePairs[MAX_MSG] = "200";


        if (send(sock, msgDemandePairs, strlen(msg), 0) < 0) { //demande liste de pair connectés
            perror("erreur envoi");
            exit(EXIT_FAILURE);
        }

        char msgListePairs[MAX_MSG];
        if (recv(sock, msgListePairs, MAX_MSG, 0) < 0) {
            perror("erreur reception");
            exit(EXIT_FAILURE);
        }

        //if (strcmp(msgListePairs, "") == 0)


    } else {
        printf("Fichier introuvable");
    }
}*/


//FONCTIONS ----------------------------------------------------

char* getFichier (char* cle) {
    char* contenu = malloc(sizeof(MAX_MSG));

    char* fileNameData;
    strcpy(fileNameData, cle);
    strcat(fileNameData, ".part");

    FILE *f = NULL;
    f = fopen(fileNameData, "r");


    if (f != NULL) {
        char tmp[8] = "";
        int car = 0;
        do {
            car = fgetc(f);
            sprintf(tmp,"%d", car);
            strcat(contenu, tmp);
            tmp[0] ='\0';

        } while (car != EOF);

        fclose(f);
    }
    return contenu;
}

ListePair* getListePairFichier(char* metadata) {
    ListePair *pairs = malloc(sizeof(*pairs));
    Pair *pair = malloc(sizeof(*pair));
    char* str = strtok (metadata, "\n");
    pair->data = str;
    pair->suiv = NULL;
    pairs->prem = pair;
    while (str != NULL) {
        str = strtok (NULL, "\n");
        Pair *p = pairs->prem;
        int added = 0;
        while (!added) {
            if(p->suiv == NULL) {
                p->suiv->data = str;
                added = 1;
            } else {
                p = p->suiv;
            }
        }
    }
    return pairs;
}

void enregistrerFichier(char* nom, char* contenu) {
    FILE *f = NULL;
    f = fopen(nom, "w");
    if (f != NULL) {
        fputs(contenu, f);
        fclose(f);
    }
}

int possedeFichier(char* cleFichier) {
    char* fileNameData;
    strcpy(fileNameData, cleFichier);
    strcat(fileNameData, ".data");

    char* fileNamePart;
    strcpy(fileNamePart, cleFichier);
    strcat(fileNamePart, ".part");

    FILE *fd = NULL;
    fd = fopen(fileNameData, "r");
    FILE *fp = NULL;
    fp = fopen(fileNamePart, "r");

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
        commande = strtok(cmd, " ");
        arg = strtok(NULL, " ");

        if (strcmp(commande, "export") == 0) {
            //printf("export ok\n");
            //pthread_create(&th, NULL, exporterFichier, (void*) arg);

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

    char msgOk[MAX_MSG];

    if (recv(sock, msgOk, MAX_MSG, 0) < 0) {
        perror("erreur reception");
        exit(EXIT_FAILURE);
    }

    if (strcmp(msgOk, "901")) { //OK
        char msgCle[MAX_MSG];
        char* clePair= malloc(sizeof(TAILLE_CLE_PAIR));
        clePair = getClePair();
        if (clePair != "") { // si deja clé
            strcat(msgCle, "101 ");
            strcat(msgCle, clePair);

            if (send(sock, msgCle, strlen(msgCle), 0) < 0) {
                perror("erreur envoi");
                exit(EXIT_FAILURE);
            }
            //attente du msg 901
            char msgConfirm[MAX_MSG];
            if (recv(sock, msgConfirm, MAX_MSG, 0) < 0) {
                perror("erreur reception");
                exit(EXIT_FAILURE);
            }
            if (strcmp(msgConfirm, "901") == 0) {
                printf("connecté au serveur avec la clé pair: %s", clePair);
            }

        } else {
            //nouveau pair
            strcat(msgCle, "102");
            if (send(sock, msgCle, strlen(msgCle), 0) < 0) {
                perror("erreur envoi");
                exit(EXIT_FAILURE);
            }

            char msgNewCle[MAX_MSG];
            if (recv(sock, msgNewCle, MAX_MSG, 0) < 0) {
                perror("erreur reception");
                exit(EXIT_FAILURE);
            }
            //nouvelle clé envoyée
            if (strcmp(msgNewCle, "103") == 0) {
                char* newCle = strtok(msgNewCle, " ");
                if (enregistrerNouvelleCle(newCle)) {
                    printf("une clé de pair a été attribuée: %s", newCle);
                } else {
                    printf("erreur de creation d'un nouveau fichier clepair");
                }
            }
        }
    }

    if (close(sock) < 0) {
		perror("erreur close");
		exit(EXIT_FAILURE);
	}
}

char* getClePair() {
    char* cle = malloc(sizeof(TAILLE_CLE_PAIR));
    FILE* f = NULL;
    f = fopen("clepair", "r");
    if (f != NULL) {
        fgets(cle, TAILLE_CLE_PAIR, f);
        fclose(f);
    }
    return cle;
}

int enregistrerNouvelleCle(char* cle) {
    FILE *f = NULL;
    f = fopen("clepair", "w");
    if (f != NULL) {
        fputs(cle, f);
        fclose(f);
    }
    return f != NULL;
}






