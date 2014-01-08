#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <regex.h>

#include "fileData.h"
#include "pair.h"

#define MSG_BUFFER_SIZE 1024

//Double linked list contenant les associations addrIP/clépair.
extern pair* ptrPair;

/**
 * Permet d'accepter un nouveau client (code message 100).
 * @param sdClient
 * @param msg
 * @return 
 */
int initClientConnect(int sdClient, struct sockaddr_in client_addr) {

    char buffer[MSG_BUFFER_SIZE];
    char** res = NULL;
    int ret = -1;
    recv(sdClient, buffer, MSG_BUFFER_SIZE, 0);

    //Preparation de l'extraction de l'addresse
    res = extract(buffer, PATTERN_IP_PORT);
    char* addrIp = NULL;
    addrIp = (char *) inet_ntoa(client_addr.sin_addr.s_addr);
    int port = atoi(res[4]);
    freeExtract(res, 5);

    //Envoi message ok.
    memset(buffer, '\0', strlen(buffer));
    strcat(buffer, "901\0");
    ret = send(sdClient, buffer, strlen(buffer), 0);
    if (ret != strlen(buffer)) {
        perror("Message can't be sent.\n");
        pthread_exit(NULL);
    }

    //Reception messages suivant.
    memset(buffer, '\0', MSG_BUFFER_SIZE);
    recv(sdClient, buffer, MSG_BUFFER_SIZE - 1, 0);

    if (strncmp(buffer, "101", 3) == 0) {
        //TO DO fonction pour ajouter une correspondance cléPair ip.
        res = NULL;
        res = extract(buffer, PATTERN_CLE_PAIR);

        addPair(res[2], addrIp, port);
        freeExtract(res, 3);

        memset(buffer, 0, strlen(buffer));
        strcat(buffer, "901");
        send(sdClient, buffer, strlen(buffer), 0);

    } else if (strncmp(buffer, "102", 3) == 0) {
        int key = getNewClePair();
        
        char keys[10];
        memset(keys, 0, 10);
        sprintf(keys, "%d", key);
        addPair(keys, addrIp, port);

        memset(buffer, 0, strlen(buffer));
        sprintf(buffer, "103 %d", key);
        send(sdClient, buffer, strlen(buffer), 0);

        memset(buffer, 0, strlen(buffer));
        recv(sdClient, buffer, MSG_BUFFER_SIZE - 1, 0);
    }
    return 0;
}

/**
 * Permet de donner à un client la liste des autres client connectés.
 * @param sdClient
 * @param client_addr
 * @return 0 si ok
 */
int shareFile(int sdClient, struct sockaddr_in client_addr) {
    char buffer[MSG_BUFFER_SIZE];
    char* addrIp = NULL;
    char **res = NULL;
    int cle = 0, totalLenght = 0;
    pair* curPtr = NULL;
    char* addrFile = NULL;
    int newCleFile = 0;

    addrIp = (char *) inet_ntoa(client_addr.sin_addr.s_addr);

    memset(buffer, 0, MSG_BUFFER_SIZE);
    recv(sdClient, buffer, 1, 0);//On enleve l'espace
    memset(buffer, 0, MSG_BUFFER_SIZE);
    recv(sdClient, buffer, MSG_BUFFER_SIZE, 0);//On lit le cleClient
    
    cle = atoi(buffer);
    
    //Préparation du message de retour.
    //envoi message "201 newCleFile "+"addr/port\n"....
    //On calcule d'abord la longueur totale du message, que l'on stocke dans totalLenght.
    for (curPtr = ptrPair; curPtr != NULL; curPtr = curPtr->next) {
        if (curPtr->clePair != cle) {
            totalLenght += strlen(curPtr->addrPair); //Longueur de cette addresseIp
            totalLenght++; // le "/" pour séparateur.
            memset(buffer, 0, MSG_BUFFER_SIZE);
            sprintf(buffer, "%d", curPtr->portPair);
            totalLenght += strlen(buffer); // longueur du port d'ecoute du pair.
            totalLenght++; //\n final entre 2 pairs.
        }
    }
    totalLenght += 5; //ajout du "201 " au début et "\0" à la fin.

    //On ajoute la nouvelle newCleFile cleFile
    newCleFile = getNewCleFile();
    memset(buffer, 0, MSG_BUFFER_SIZE);
    sprintf(buffer, "%d", newCleFile);
    totalLenght += strlen(buffer);

    totalLenght++; //Ajout de l'espace entre newCleFile et addr

    //On alloue la mémoire.
    addrFile = malloc(sizeof (char) * totalLenght);
    memset(addrFile, 0, totalLenght);

    //On crée le message de retour.
    strcat(addrFile, "201 ");
    strcat(addrFile, buffer);
    strcat(addrFile, " ");
    for (curPtr = ptrPair; curPtr != NULL; curPtr = curPtr->next) {
        if (curPtr->clePair != cle) {
            strcat(addrFile, curPtr->addrPair); //addresseIp
            strcat(addrFile, "/"); // "/" séparateur.
            memset(buffer, 0, MSG_BUFFER_SIZE);
            sprintf(buffer, "%d", curPtr->portPair);
            strcat(addrFile, buffer); //port
            strcat(addrFile, "\n");
        }
    }


    send(sdClient, addrFile, totalLenght, 0);
    free(addrFile);

    //Reception du fichier de meta-data de retour.
    memset(buffer, 0, MSG_BUFFER_SIZE);
    recv(sdClient, buffer, 4, 0);
    if (strncmp(buffer, "202 ", 3) == 0) {//On recois le nouveau message de demande d'envoi des metadata. 
        newMetaDataFile(newCleFile, sdClient); //On sauve le nouveau metadata envoyé sur le server.
        memset(buffer, 0, MSG_BUFFER_SIZE);
        
        sprintf(buffer, "901 ");
    }

    return 0;
}

/**
 * Permet de donner à un client la liste des autre clients possédant un partis du fichier cléFichier. ou la liste des fichiers disponnibles à télécharger.
 * @param sdClient
 * @param client_addr
 * @return 
 */
int dlFile(int sdClient, struct sockaddr_in client_addr) {
    char buffer[MSG_BUFFER_SIZE];
    char *addrIp, *path;
    int port, totalLenght = 0;
    char** res;

    addrIp = (char *) inet_ntoa(client_addr.sin_addr.s_addr);
    port = ntohs(client_addr.sin_port);

    memset(buffer, 0, MSG_BUFFER_SIZE);
    recv(sdClient, buffer, MSG_BUFFER_SIZE, 0);

    res = extract(buffer, PATTERN_CLE_PAIR);

    //Création du path du fichier à ouvrir
    if (metaFileExist(res[2]) == 1) {//Si le fichier existe, on recupère le contenu et on l'envoi.
        sendMetaFile(sdClient, res[2], "301 ");
    }
    freeExtract(res, 3);
    return 0;
}

/**
 * Extrait les données d'un message suivant un pattern et les retourne dans un tableau.
 * @param msg
 * @param pattern
 * @return 
 */
char** extract(const char* msg, const char* pattern) {
    int err;
    regex_t preg;
    int match;
    size_t nmatch = 0;
    regmatch_t *pmatch = NULL;
    char **res = NULL;

    const char *str_request = msg;
    const char *str_regex = pattern;

    if (regcomp(&preg, str_regex, REG_EXTENDED) != 0) {
        perror("Regcomp error.");
        exit(EXIT_FAILURE);
    }

    nmatch = preg.re_nsub;
    pmatch = malloc(sizeof (regmatch_t) * nmatch);
    if (pmatch) {
        match = regexec(&preg, str_request, nmatch, pmatch, 0);
        regfree(&preg);

        if (match == 0) {
            int start;
            int end;
            size_t size;
            res = malloc(sizeof (char*) * nmatch);
            int i;
            for (i = 0; i < nmatch; i++) {
                start = pmatch[i].rm_so;
                end = pmatch[i].rm_eo;
                size = end - start;
                res[i] = malloc(sizeof (char) * (size + 1));
                strncpy(res[i], &str_request[start], size);
                res[i][size] = '\0';
            }
            return res;
        } else if (match == REG_NOMATCH) {
            printf("%s n\'est pas un message \"addrpIP/port\" valide\n", str_request);
        } else {
            char *text;
            size_t size;

            size = regerror(err, &preg, NULL, 0);
            text = malloc(sizeof (*text) * size);
            if (text) {
                regerror(err, &preg, text, size);
                fprintf(stderr, "%s\n", text);
                free(text);
            } else {
                fprintf(stderr, "Memoire insuffisante\n");
                exit(EXIT_FAILURE);
            }
        }
    } else {
        fprintf(stderr, "Memoire insuffisante\n");
        exit(EXIT_FAILURE);
    }
    return res;
}

/**
 * Permet de libérer la mémoire utilisée par la fonction extract et retourné dans un char**
 * @param res tableau à libérer
 * @param nmatch nombre d'élément du tableau (première dimension)
 * 
 * @return 0 si ok -1 si probleme
 */
int freeExtract(char** res, int nmatch) {
    if (res == NULL) {
        return 0;
    }
    int i = 0;
    for (i = 0; i < nmatch; i++) {
        free(res[i]);
    }
    free(res);
    res = NULL;
    return 0;

}


/*******************************************
 Fonctions pour gérer la liste des pairs.
 *******************************************/

/**
 * Ajoute un element à pair si il n'existe pas ou met à jour l'élément correspondant si il existe déjà.
 * @param clePair du pair à ajouter/màj
 * @param ip du pair
 * @param port du pair
 * @return 0 si tout vas bien, -1 sinon.
 */
int addPair(const char* clePair, const char* ip, const int port) {
    pair* ptr;
    int cle = atoi(clePair);
    ptr = findPair(cle);

    //Si l'élément existe déjà.
    if (ptr != NULL) {
        updatePair(ptr, ip, port);
        return 0;
    }

    //Si il n'y a aucun element.
    if (ptrPair == NULL) {
        ptrPair = (pair*) malloc(sizeof (struct pair));

        ptrPair->clePair = cle;
        ptrPair->addrPair = malloc(sizeof (char) * (strlen(ip) + 1));
        strcpy(ptrPair->addrPair, ip);

        ptrPair->portPair = port;
        ptrPair->next = NULL;
        ptrPair->prev = NULL;
        return 0;
    }

    if (findNearbyElem(cle, &ptr) < 0) {
        printf("Erreur findNearbyElem.");
        exit(EXIT_FAILURE);
    }
    
    if (cle > (ptr->clePair)) {//Si on doit placer l'élément après
        ptr->next = malloc(sizeof (pair));

        (ptr->next)->prev = ptr;
        ptr = ptr->next;

        ptr->clePair = cle;
        ptr->addrPair = malloc(sizeof (char) * (strlen(ip) + 1));
        strcpy(ptr->addrPair, ip);

        ptr->portPair = port;
        ptr->next = NULL;
        return 0;
    } else if ((cle < (ptr->clePair)) && (ptrPair == ptr)) {//Si le premier élément de la liste a une clé deja supérieure.
        ptr->prev = malloc(sizeof (pair));

        (ptr->prev)->next = ptr;
        ptr = ptr->prev;
        ptr->clePair = cle;
        ptr->addrPair = malloc(sizeof (char) * (strlen(ip) + 1));
        strcpy(ptr->addrPair, ip);
        ptr->portPair = port;
        ptr->prev = NULL;
        ptrPair = ptr;
        return 0;
    } else if (cle < (ptr->clePair)) {//Si l'on doit l'inserer au milieu de la liste (le pointeur étant sur l'élément suivant).
        pair *next, *prev;
        next = ptr;
        prev = ptr->prev;

        ptr->prev = malloc(sizeof (pair));
        ptr = ptr->prev;
        prev->next = ptr;

        ptr->clePair = cle;
        ptr->addrPair = malloc(sizeof (char) * (strlen(ip) + 1));
        strcpy(ptr->addrPair, ip);

        ptr->portPair = port;
        ptr->next = next;
        ptr->prev = prev;
        return 0;
    }
    return -1;
}

/**
 * Recherche un pair dans la liste des pairs.
 * Si elle le trouve elle retourne son addresse.
 * Sinon elle retourne NULL.
 * @return 
 */
pair * findPair(const int clePair) {
    pair* ptr = NULL;

    if (ptrPair == NULL)
        return NULL;

    for (ptr = ptrPair; ptr->clePair < clePair && ptr->next != NULL; ptr = ptr->next)
        ;

    if (ptr->clePair != clePair)
        return NULL;

    return ptr;
}

pair * updatePair(pair* ptr, const char* ip, const int port) {
    //On libere la memoire si déja prise.
    if (ptr->addrPair != NULL) {
        free(ptr->addrPair);
    }

    //Maj addr
    ptr->addrPair = malloc(sizeof (char) * (strlen(ip) + 1));
    strcpy(ptr->addrPair, ip);

    //Maj ptr
    ptr->portPair = port;

    return ptr;
}

/**
 * Met dans ptr l'addresse d'un élément qui devrait etre adjacent à celui à ajouter.
 * Si il n'y a pas d'élem retourne -1, si l'élem existe déja retourne -2.
 * @param clePair
 * @param offset
 * @param ptr
 * @return 0 si ok -1 si probleme.
 */
int findNearbyElem(const int clePair, pair** ptr) {

    *ptr = ptrPair;
    if (ptrPair == NULL) {
        return -1;
    }
    while ((((*ptr)->clePair) < clePair) && (((*ptr)->next) != NULL)) {
        (*ptr) = ((pair*)(*ptr)->next);
    }
    return 0;
}
