#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define MAX_MSG 1024
#define MAX_CONNEXIONS 10

void* ecouteRequetes(void* arg);

pthread_t lancerDaemon();
void inviteDeCommandes();



int main () {
    //lancer daemon
    //pthread_t daemon = lancerDaemon();
    inviteDeCommandes();
}

//THREAD
void* ecouteRequetes(void* arg) {
    int desClients[MAX_CONNEXIONS];
    pthread_t th[MAX_CNNEXIONS]
    int brPub = socket(PF_INET, SOCK_STREAM, 0);
    if (brPub == -1) {
        cout << "erreur socket" << endl;
        exit(EXIT_FAILURE);
    }

    sockaddr_in add;
    add.sin_family = AF_INET;
    add.sin_addr.s_addr = htonl(INADDR_ANY);
    add.sin_port = htons(44444);

    int errlisten = listen(brPub, 1);

    struct sockaddr_in brC;
    socklen_t sizebrC = sizeof(brC);

    int i = 0;
    while (i < MAX_CONNEXIONS) {
        desClients[i] = accept((int)brPub, (struct sockaddr *)&brC, &sizebrC);
        if (desClients[i] < 0) {
            //printf("erreur accept");
        } else {
            char msg[MAX_MSG];
            if (recv(cli, msg, MAX, 0) < 0) {
                //printf ("err")
            } else {
                char* part = strtok(msg, " ");
                char* numMsg = part;
                part = strtok(NULL, " ");
                char* arg = part;
                if (strcmp("302", numMsg) == 0) { // as tu une partition de ce fichier
                    //TODO passer 2 param: descClients[i] et arg (cle fichier)
                    pthread_create(th[i], NULL, demandePartition, (void*)desClients[i]);
                }
            }
        }
    }
}

void* demandePartition(void* arg) {

}

//FONCTIONS

pthread_t lancerDaemon() {
    pthread_t d;
    if (pthread_create(&d, NULL, ecouteRequetes, NULL) < 0) {
        printf("erreur creation daemon");
    }
    return d;
}

void inviteDeCommandes() {
    char cmd[50];
    int exit = 0;
    while (exit != 1) {
        scanf("%s", cmd);
        char* part = strtok(cmd, " ");
        printf("%s", part);
        //char* deb = part;
        while (part != NULL) {
            printf("%s", part);
            part = strtok(NULL, " ");
        }
        /*printf("part: ");
        printf("%s\n", part);
        printf("CMD: ");
        printf("%s\n", cmd);*/
        //recuperer path
        if (strcmp(part, "export") == 0) {
            printf("export ok\n");

        } else if (strcmp(part, "download") == 0) {
            printf("download ok\n");
            char* deb = part;
            char* fichier = strtok(cmd, " ");
        } else if (strcmp(part, "exit") == 0) {
            exit = 1;
            printf("fin\n");
        } else {
            printf ("commande non reconnue\n");
        }
        cmd[0] = '\0';
    }
}


