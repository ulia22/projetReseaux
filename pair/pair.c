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
#include <sys/ioctl.h>

#include "pair.h"

#define MAX_MSG 1024
#define MAX_CONNEXIONS 10
//#define PORT_CLIENT 44443
#define TAILLE_CLE_PAIR 10

void* ecouteRequetes(void* arg);
void* gestionConnexion(void* arg);
void* importerFichier(void* arg);
void* obtenirPartition(void* arg);

int enregistrerPartition(char* cleFichier, char* contenu);
char* envoyerPartition(char* cleFichier, char* path, struct sockaddr_in add);
char* creerMetaData(struct sockaddr_in add);
char* getFichier(char* path);
char* getFichierParCle (char* cle);
ListePair* getListePairFichier(char* metadata);
void enregistrerFichier(char* nom, char* contenu);
int possedeFichier(char* cleFichier);
pthread_t lancerDaemon();
void inviteDeCommandes();
void initConnexion(char* ip, char* port);
int getIP(int fd, char* ifname, u_int32_t *ipaddr);
char* getClePair();
int enregistrerNouvelleCle(char* cle);

char ip_server[16];
char port_server[7];
extern int port_client;

int main (int argc, char *argv[]) {
    //lancer daemon
	if(argc < 4){
		printf("Entrer le nom du programme suivit de l'addresse ip du server, de son port d'ecoute et du port d'ecoute du client.\n");
		printf("Exemple: ./pair 127.0.0.1 44444 44443\n");
		exit(EXIT_FAILURE);
	}
	port_client = atoi((char*)argv[3]);
    initConnexion(argv[1], argv[2]);

    pthread_t daemon = lancerDaemon();
    inviteDeCommandes();
}

//THREAD -----------------------------------------------------------------

//-------DEAMON-------------------------------------

void* ecouteRequetes(void* arg) { //deamon
    int desClient;
    pthread_t th;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("erreur socket\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in add;
    add.sin_family = AF_INET;
    add.sin_addr.s_addr = htonl(INADDR_ANY);
    add.sin_port = ntohs(port_client);


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
            pthread_create(&th, NULL, gestionConnexion, (void*)desClient);
        }
    }
    if (close(sock) < 0) {
                perror("erreur close 1");
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
        char* numMsg;
        char* argm;
	char* content;
	numMsg = strtok(msg, " ");
	argm = strtok(NULL, " ");
	content = strtok(NULL, "\0");

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
                strcat(partition, getFichierParCle(argm));
                if(send(desCli, partition, strlen(partition), 0) < 0) { //envoi du message 501 contenudufichier
                    perror("erreur send");
                    exit(EXIT_FAILURE);
                }
            }
        } else if(strncmp(numMsg, "600", 3) == 0) { // demande d'un pair de stockage d'une partition

            if (enregistrerPartition(argm, content)) {
                char msgRecu[MAX_MSG] = "601";
                if(send(desCli, msgRecu, strlen(msgRecu), 0) < 0) { //envoi du message 601 de confirmation de stockage de la partition
                    perror("erreur send");
                    exit(EXIT_FAILURE);
                }
            } else {
                char msgEchec[MAX_MSG] = "602";
                if(send(desCli, msgEchec, strlen(msgEchec), 0) < 0) {
                    perror("erreur send");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    pthread_exit(NULL);
}

//-----------DOWNLOAD--------------------------------

void* importerFichier(void* arg) {

    char* cleFichier = (char*)arg;
    pthread_t th;

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("erreur socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in add;
    add.sin_family = AF_INET;
    add.sin_addr.s_addr = inet_addr(ip_server);
    add.sin_port = ntohs(atoi(port_server));

    if (connect(sock, (struct sockaddr*)&add, sizeof(add)) < 0) {
        perror("erreur connect");
        exit(EXIT_FAILURE);
    }

    char msg[MAX_MSG];
    strcat(msg, "300 ");
    strcat(msg, cleFichier);
    if (send(sock, msg, strlen(msg), 0) < 0) { // demande liste des pairs ayant une partition du fichier arg
        perror("erreur envoi");
        exit(EXIT_FAILURE);
    }


    char msgRecv[MAX_MSG];

    if (recv(sock, msgRecv, MAX_MSG, 0) < 0) {
        perror("erreur reception");
        exit(EXIT_FAILURE);
    }
    char* numMsg = strtok(msgRecv, " ");
    if (strcmp("301", numMsg) == 0) { // reception de la liste des pairs ayant une partition
        char* metadata = strtok(NULL, "\n");
        ListePair *pairs = getListePairFichier(metadata);


        Pair *p = pairs->prem;
        char* part;
        char* ip;
        char* port;
        int recu = 0;
	struct sockaddr_in addPair;
        //int part_traites[] = {0}
        //TODO gerer les partitions : recuperer le nombre de partitions
        while (p != NULL && !recu) {

            part = strtok(p->data, " ");
            ip = strtok(NULL, " ");
            port = strtok(NULL, " ");

            
	    memset(&addPair, 0, sizeof(struct sockaddr_in));
            addPair.sin_family = AF_INET;
            addPair.sin_addr.s_addr = inet_addr(ip);
            addPair.sin_port = htons(atoi(port));

            ParamPartition *params = (ParamPartition *)malloc(sizeof(ParamPartition));
            params->add = addPair;
	    params->cleFichier = malloc(strlen(cleFichier)+1);
	    memset(params->cleFichier, '\0', strlen(cleFichier)+1); 
            params->cleFichier = cleFichier; // TODO : recuperer la clé du fichier plutot que le nom (faire en sorte que le serveur l'envoi ?

            pthread_create(&th, NULL, obtenirPartition, (void*)params);
            void* partition;
            if (pthread_join(th, &partition) != 0) {
                perror("erreur join");
                exit(EXIT_FAILURE);
            }
            if((char*)partition != "") {
                enregistrerFichier(cleFichier, (char*)partition);
                recu = 1;
            }
            p = p->suiv;
        }
        if (!recu) { // pas de fichier telechargé
            printf("Impossible de telecharger le fichier\n");
        }
    }
    if (close(sock) < 0) {
                perror("erreur close 2");
                exit(EXIT_FAILURE);
        }
    printf("Fichier téléchargé.\n");
    pthread_exit(NULL);
}

void* obtenirPartition(void* arg) {
    ParamPartition *params = (ParamPartition*)arg;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror( "erreur socket");
        exit(EXIT_FAILURE);
    }
    if (connect(sock, (struct sockaddr*)(&(params->add)), sizeof(params->add)) != 0) {
        perror("erreur connect obtenir partition.");
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
    memset(msgPartition, '\0', MAX_MSG);

    if (recv(sock, msgPartition, MAX_MSG, 0) < 0) { // reception de la partition
        perror("erreur reception");
        exit(EXIT_FAILURE);
    }
    char* partition = malloc(sizeof(char) * MAX_MSG);

    char* codeMsg = strtok(msgPartition, " ");
    if (strcmp(codeMsg, "501") == 0) { //
         partition = strtok(NULL, "\0");
    }
        if (close(sock) < 0) {
                perror("erreur close 3");
                exit(EXIT_FAILURE);
        }
    pthread_exit((void*)partition);
}

//---------------UPLOAD-------------------------------------

void* exporterFichier(void* path) {
    char* nom = (char*)path;
    char* metadata = "";

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("erreur socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in add;
    add.sin_family = AF_INET;
    add.sin_addr.s_addr = inet_addr(ip_server);
    add.sin_port = ntohs(atoi(port_server));

    if (connect(sock, ((struct sockaddr *)(struct sock_addr_in *)(&add)), sizeof(add)) < 0) {
        perror("erreur connect");
        exit(EXIT_FAILURE);
    }

    //Message "200 notreClePair"
    char msgDemandePairs[MAX_MSG] = "200 ";
    strcat(msgDemandePairs, strtok(getClePair(), "\n"));

    if (send(sock, msgDemandePairs, strlen(msgDemandePairs), 0) < 0) { //demande liste de pair connectés
        perror("erreur envoi");
        exit(EXIT_FAILURE);
    }

    char msgListePairs[MAX_MSG];//Recupère le message avec la liste des pair depuis le server
    if (recv(sock, msgListePairs, MAX_MSG, 0) < 0) {
        perror("erreur reception");
        exit(EXIT_FAILURE);
    }

    char* codeMsg = strtok(msgListePairs, " ");
    if (strcmp(codeMsg, "201") == 0) { //liste des connectes

        char* cleFichier = strtok(NULL, " "); //cle fichier génèrer par le serveur
        char* pairs = strtok(NULL, " ");
        ListePair *listeP = getListePairFichier(pairs);;
        int envoye = 0;
        char *ip = "";
        char *port = "";
        Pair *p = listeP->prem;
        do {

            ip = strtok(p->data, "/");
            port = strtok(NULL, "\n");

            struct sockaddr_in addPair;
            addPair.sin_family = AF_INET;
            addPair.sin_addr.s_addr = inet_addr(ip);
            addPair.sin_port = ntohs(atoi(port));
            metadata = envoyerPartition(cleFichier, path, addPair);
            if(metadata != NULL) {
                envoye = 1; //fichier envoye
            } else { //echec, on passe a un autre pair
                p = p->suiv;
            }

        } while(p->suiv != NULL && !envoye);
        if(envoye) {
            //on previens le serveur d'un nouveau metadata et on lui envoie
            char msgMetaData[MAX_MSG] = "202 ";
            strcat(msgMetaData, metadata);
            if (send(sock, msgMetaData, strlen(msgMetaData), 0) < 0) {
                perror("erreur envoi");
                exit(EXIT_FAILURE);
            }
            printf("Fichier exporté avec succès\n");
        } else {
            printf("Impossible d'envoyer le fichier\n");
        }
    }
    if (close(sock) < 0) {
                perror("erreur close upload");
                exit(EXIT_FAILURE);
        }
        pthread_exit(NULL);
}


//FONCTIONS ----------------------------------------------------
/*
enregistrement d'une partition recue
return 1 si succes, 0 sinon
*/
int enregistrerPartition(char* cleFichier, char* contenu) {
    FILE *f = NULL;
    char fileName [256];
    memset(fileName, 0, 256);
    strcat(fileName, cleFichier);
    strcat(fileName, ".part");
    f = fopen(fileName, "w");
    if (f != NULL) {
        fprintf(f, "%s",contenu);
         if(fclose(f) != 0){
		perror("FClose error 1");
		exit(EXIT_FAILURE);
	}
        return 1;
    } else {
        return 0;
    }
}

/*
retourne metadata si succes, NULL sinon
*/
char* envoyerPartition(char* cleFichier, char* path, struct sockaddr_in add) {
    char* metadata;
    char* contenu = getFichier(path);

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("erreur socket");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr*)&add, sizeof(add)) != 0) {
        perror("erreur connect");
        exit(EXIT_FAILURE);
    }

    char msgPartition[MAX_MSG] = "600 ";
    strcat(msgPartition, cleFichier);
    strcat(msgPartition, " ");
    strcat(msgPartition, contenu);
    free(contenu);

    if (send(sock, msgPartition, strlen(msgPartition), 0) < 0) { //envoi du message envoi d'une partition a stocker
        perror("erreur envoi");
        exit(EXIT_FAILURE);
    }
    char rep[MAX_MSG];
    if (recv(sock, rep, MAX_MSG, 0) < 0) {
            perror("erreur reception");
            exit(EXIT_FAILURE);
    }
    if (strncmp(rep, "601", 3) == 0) {
        char* md = creerMetaData(add);
        if (close(sock) < 0) {
            perror("erreur close envoyerPartiton 601");
            exit(EXIT_FAILURE);
        }

        return md;//succes
    } else if (strncmp(rep, "602", 3) == 0) {
        if (close(sock) < 0) {
            perror("erreur close envoyerPartiton 602");
            exit(EXIT_FAILURE);
        }
        return NULL; //echec
    }
}

/*
creer et retourne metadata de la forme <numero de la parition> <ip> <port>\n
*/
char* creerMetaData(struct sockaddr_in add) {

    char* metadata = malloc(sizeof(char) * MAX_MSG);
    memset(metadata, 0, MAX_MSG);
    sprintf(metadata, "1 %s %d\n", inet_ntoa(add.sin_addr), ntohs(add.sin_port));
    return metadata;
}

char* getFichier(char* path) {
    char* contenu = malloc(sizeof(char) * MAX_MSG);
    memset(contenu, 0, MAX_MSG);
    FILE *f = NULL;
    f = fopen(path, "r");

    char tmp;
    if (f != NULL) {
        int car = 0, i = 0;
        while((car = fgetc(f)) != EOF){
            contenu[i] = car;
            i++;
        }
        contenu[i] = '\0';
         if(fclose(f) != 0){
		perror("FClose error 2");
		exit(EXIT_FAILURE);
	}
    }
    return contenu;
}

char* getFichierParCle (char* cle) {
    char* contenu = malloc(sizeof(char) * MAX_MSG);
    memset(contenu, 0, sizeof(char) * MAX_MSG);

    char* fileNameData = malloc(sizeof(char) * (strlen(cle) + strlen(".part") + 1));
    memset(fileNameData, '\0', sizeof(char) * (strlen(cle) + strlen(".part") + 1));
    strcpy(fileNameData, cle);
    strcat(fileNameData, ".part");

    FILE *f = NULL;
    f = fopen(fileNameData, "r");


    if (f != NULL) {
        char tmp[8];
	memset(tmp, 0, sizeof(char) * 8);
        int car = 0;
        while ((car = fgetc(f)) != EOF){
            sprintf(tmp, "%c", car);
            strcat(contenu, tmp);
            memset(tmp, 0, sizeof(char) * 8);

        }
	free(fileNameData);
         if(fclose(f) != 0){
		perror("FClose error 3");
		exit(EXIT_FAILURE);
	}
    }
    return contenu;
}

ListePair* getListePairFichier(char* metadata) {
    ListePair *pairs = malloc(sizeof(ListePair));
    Pair *pair = malloc(sizeof(Pair));
    char* str = strtok (metadata, "\n");
    pair->data = str;
    pair->suiv = NULL;
    pairs->prem = pair;
    while (str != NULL) {
        Pair *p = pairs->prem;
        int added = 0;
        while (!added) {
            if(p->suiv == NULL) {
                p->data = str;
                added = 1;
            } else {
                p = p->suiv;
            }
        }
        str = strtok (NULL, "\n");
    }
    return pairs;
}

void enregistrerFichier(char* nom, char* contenu) {
    FILE *f = NULL;
    f = fopen(nom, "w");
    if (f != NULL) {
        fputs(contenu, f);
         if(fclose(f) != 0){
		perror("FClose error 4");
		exit(EXIT_FAILURE);
	}
    }
}

int possedeFichier(char* cleFichier) {
    char fileNameData[256];
    memset(fileNameData, '\0', 256);
    strcpy(fileNameData, cleFichier);
    strcat(fileNameData, ".data");

    char fileNamePart[256];
    memset(fileNamePart, '\0', 256);
    strcpy(fileNamePart, cleFichier);
    strcat(fileNamePart, ".part");

    FILE *fd = NULL;
    fd = fopen(fileNameData, "r");

    FILE *fp = NULL;
    fp = fopen(fileNamePart, "r");
    if (/*fd != NULL && */fp != NULL) {
        /*if(fclose(fd) != 0){
		perror("FClose error fd possederFichier");
		exit(EXIT_FAILURE);
	}*/
        if(fclose(fp) != 0){
		perror("FClose error fp possederFichier");
		exit(EXIT_FAILURE);
	}
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

        scanf("%[a-z 0-9 ' ']", cmd);
        commande = strtok(cmd, " ");
        arg = strtok(NULL, " ");

        if (strcmp(commande, "export") == 0) {
            pthread_create(&th, NULL, exporterFichier, (void*) arg);

        } else if (strcmp(commande, "import") == 0) {

            pthread_create(&th, NULL, importerFichier, (void*) arg);
        } else if (strcmp(commande, "exit") == 0) {
            exit = 1;
            printf("Fin du programme\n");
        } else {
            printf ("commande non reconnue\n");
        }
        cmd[0] = 0;
        while(getchar()!='\n');
    }
}

void initConnexion(char* ip, char* port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("erreur socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in add;
    memset(&add, 0, sizeof(struct sockaddr_in));
    add.sin_family = AF_INET;
    add.sin_addr.s_addr = inet_addr(ip);
    add.sin_port = htons(atoi(port));

    memset(ip_server, 0, 16);
    memset(port_server, 0, 7);

    sprintf(ip_server, "%s", ip);
    sprintf(port_server, "%s", port);


    if (connect(sock, (struct sockaddr*)&add, sizeof(add)) < 0) {
        perror("erreur connect");
        exit(EXIT_FAILURE);
    }

    //recuperation de notre ip
    u_int32_t ipp;
    getIP(sock, "eth0", &ipp);
    struct in_addr in;
    in.s_addr = ipp;
    char* cip = inet_ntoa(in);


    char msg[MAX_MSG] = "100 ";

    strcat(msg, cip);

    strcat(msg, "/");

    //port
    char tmp_port[7];
    sprintf(tmp_port, "%d", port_client);
    strcat(msg, tmp_port);


    if (send(sock, msg, strlen(msg), 0) < 0) {
        perror("erreur envoi");
        exit(EXIT_FAILURE);
    }

    char msgOk[MAX_MSG];

    if (recv(sock, msgOk, MAX_MSG, 0) < 0) {
        perror("erreur reception");
        exit(EXIT_FAILURE);
    }

    if (strcmp(msgOk, "901") == 0) { //OK
        char msgCle[MAX_MSG];
        char* clePair = malloc(sizeof(char) * TAILLE_CLE_PAIR);
	memset(clePair, 0, sizeof(char) * TAILLE_CLE_PAIR);
        clePair = getClePair();
        if (clePair != NULL) { // si deja clé
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
                printf("connecté au serveur avec la clé pair: %s\n", clePair);
            }
            free(clePair);
        } else {
            //nouveau pair
            strcat(msgCle, "102");
            if (send(sock, msgCle, strlen(msgCle), 0) < 0) {
                perror("erreur envoi");
                exit(EXIT_FAILURE);
            }

            char msgNewCle[MAX_MSG];
	    memset(msgNewCle, 0, sizeof(char)*MAX_MSG);
            if (recv(sock, msgNewCle, MAX_MSG, 0) < 0) {
                perror("erreur reception");
                exit(EXIT_FAILURE);
            }

            //nouvelle clé envoyée
            char* codeMsg = strtok (msgNewCle, " ");
            if (strcmp(codeMsg, "103") == 0) {
                char* newCle = strtok(NULL, " ");
                if (enregistrerNouvelleCle(newCle)) {
                    printf("une clé de pair a été attribuée: %s\n", newCle);
                } else {
                    printf("erreur de creation d'un nouveau fichier clepair\n");
                }
            }
        }
    }

    if (close(sock) < 0) {
                perror("erreur close 3");
                exit(EXIT_FAILURE);
        }
}

int getIP(int fd, char* ifname, u_int32_t *ipaddr) {

    struct ifreq {
        char        ifr_name[16];
        struct        sockaddr ifr_addr;
    };

    struct ifreq ifr;
    memcpy(ifr.ifr_name, ifname, 16);
    if(ioctl(fd, SIOCGIFADDR, &ifr) == -1) {
        perror("getIP: ioctl" );
        exit(-1);
    }
    memcpy(ipaddr, &ifr.ifr_addr.sa_data[2], 4);
    return 0;
}

char* getClePair() {
    char* cle = malloc(sizeof(TAILLE_CLE_PAIR));
    FILE* f = NULL;
    f = fopen("clepair", "r");
    if (f != NULL) {
        fgets(cle, TAILLE_CLE_PAIR, f);
        if(fclose(f) != 0){
		perror("FClose error 5");
		exit(EXIT_FAILURE);
	}

        return cle;
    } else {
        free(cle);
        return NULL;
    }



}

int enregistrerNouvelleCle(char* cle) {
    FILE *f = NULL;
    f = fopen("clepair", "w");
    if (f != NULL) {
        fputs(cle, f);
         if(fclose(f) != 0){
		perror("FClose error 6");
		exit(EXIT_FAILURE);
	}
        return 1;
    } else {
        return 0;
    }

}
