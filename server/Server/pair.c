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
pair* ptrPair;

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
    char addrIp[16]; //En cour de modif
    addrIp = (char *) inet_ntoa(client_addr.sin_addr.s_addr);
    int port = (unsigned int) ntohs(client_addr.sin_port);
    printf("Pair connecté à l'addresse: %s , et au port: %u.\n", addrIp, port);

    //Envoi message ok.
    memset(buffer, '\0', strlen(buffer));
    strcat(buffer, "901\0");
    ret = send(sdClient, buffer, strlen(buffer), 0);
    if (ret != strlen(buffer)) {
        perror("Message can't be sent.\n");
        pthread_exit(NULL);
    }
    printf("Message sent.\n");

    //Reception messages suivant.
    memset(buffer, '\0', MSG_BUFFER_SIZE);
    recv(sdClient, buffer, MSG_BUFFER_SIZE - 1, 0);

    if (strncmp(buffer, "101", 3) == 0) {
        //TO DO fonction pour ajouter une correspondance cléPair ip.
        res = NULL;
        res = extract(buffer, PATTERN_CLE_PAIR);
        int j;
        for(j = 0; j < 3; j++){
            printf("Pour j=%d res = %s\n", j, res[j]);
        }
        addPair(res[2], addrIp, port);
        if (ptrPair != NULL) {
            printf("Correspondance ajoutée:\nClé: %d \nIP: %s \nPort: %d \nPrev: %p \nNext: %p \n", (ptrPair->clePair), ptrPair->addrPair, (ptrPair->portPair), ptrPair->prev, ptrPair->next);
        } else {
            printf("ptrPair est NULL");
        }
        
        memset(buffer, 0, strlen(buffer));
        strcat(buffer, "901");
        send(sdClient, buffer, strlen(buffer), 0);
        
    }else if(strncmp(buffer, "102", 3) == 0){
        int key = getNewClePair();
        
        char keys[10];
        memset(keys, 0, 10);
        sprintf(keys, "%d", key);
        addPair(keys, addrIp, port);
        printf("%d", ptrPair->clePair);
        
        memset(buffer, 0, strlen(buffer));
        sprintf(buffer, "103 %d", key);
        send(sdClient, buffer, strlen(buffer), 0);
        
        memset(buffer, 0, strlen(buffer));
        recv(sdClient, buffer, MSG_BUFFER_SIZE - 1, 0);
        printf("last message: %s\n", buffer);
    }
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
        ptrPair = malloc(sizeof (pair));

        ptrPair->clePair = cle;

        ptrPair->addrPair = malloc(sizeof (char) * (strlen(ip) + 1));
        strcpy(ptrPair->addrPair, ip);

        ptrPair->portPair = port;
        ptrPair->next = NULL;
        ptrPair->prev = NULL;
    }

    //Si il y a déjà des éléments en place, mais pas le bon.
    if (ptr == NULL) {

        //On trouve l'element le plus proche.
        int offset = 0;
        findNearbyElem(cle, &offset, ptr);

        //Dernier elem de la liste atteint, rajouter l'elem à la fin.
        if (offset == 1) {
            ptr->next = malloc(sizeof (pair));
            (ptr->next)->prev = ptr;
            ptr = ptr->next;

            ptr->clePair = cle;

            ptr->addrPair = malloc(sizeof (char) * (strlen(ip) + 1));
            strcpy(ptr->addrPair, ip);

            ptr->portPair = port;
            ptr->next = NULL;
        } else if (offset == -1) {//Inserer le nouveau pair entre 2 autre.

            //On enregistre les addresses des elems qui seront les suivant et précédent.
            pair *nextPair, *prevPair;
            nextPair = ptr;
            prevPair = ptr->prev;

            //On créé le nouveau pair à l'addresse elemSuivant->prev.
            ptr->prev = malloc(sizeof (pair));

            //On relie elemPrec->next au nouveau pair.
            if (prevPair != NULL) {
                prevPair->next = ptr->prev;
            }


            //Finalement on pointe le nouvel elem pour remplir la structure.
            ptr = ptr->prev;

            //On fini de relie les element adjacents.
            ptr->next = nextPair;
            ptr->prev = prevPair;

            //On met la cle, l'addrIP, et le port.
            ptr->clePair = cle;

            ptr->addrPair = malloc(sizeof (char) * (strlen(ip) + 1));
            strcpy(ptr->addrPair, ip);

            ptr->portPair = port;

            //Enfin si l'element precedent est null (donc on est en premiere position)
            //On change ptrPair pour qu'il pointe toujours sur le premier element.
            if (ptr->prev == NULL) {
                ptrPair = ptr;
            }
        }
    }
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
 * Met dans ptr l'addresse d'un élément qui devrait etre adjacent à celui à ajouter,
 * avec en offset un int négatif si l'élément devra etre ajouté avant, ou positif si il dois etre ajouté apres.
 * Si il n'y a pas d'élem retourne -1, si l'élem existe déja retourne -2.
 * @param clePair
 * @param offset
 * @param ptr
 * @return 0 si ok -1 si probleme.
 */
int findNearbyElem(const int clePair, int* offset, pair * ptr) {

    ptr = NULL;
     *offset = 0;

    if (ptrPair == NULL)
        return -1;

    for (ptr = ptrPair; ptr->clePair < clePair && ptr->next != NULL; ptr = ptr->next)
        ;

    if (ptr->clePair > clePair) {
     *offset = -1;
        return 0;
    } else if (ptr->clePair < clePair && ptr->next == NULL) {
     *offset = 1;
        return 0;
    } else if (ptr->clePair == clePair) {
        return -2;
    }
    return -3;
}
