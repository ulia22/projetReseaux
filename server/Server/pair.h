/* 
 * File:   clientData.h
 * Author: cbarbaste
 *
 * Created on 3 décembre 2013, 17:07
 */

#ifndef CLIENTDATA_H
#define	CLIENTDATA_H

#ifdef	__cplusplus
extern "C" {
#endif

    typedef struct pair {
        int clePair;
        char* addrPair;
        int portPair;
        struct pair* next;
        struct pair* prev;
    }pair;

    typedef struct acceptThreadArg {
        int sock;
        struct sockaddr_in pair;
    }acceptThreadArg;


    /***********************************************
     * Gestion des correspondance clePair, addrIP.
     ***********************************************/
    //Double linked list contenant les associations addrIP/clépair.
    //ptrPair pointe sur le premier élément de la liste
    //Cette liste est ordonnée suivant clePair dans l'ordre croissant.
    pair* ptrPair;

    pair* updatePair(pair* ptr, const char* ip, const int port);
    pair* findPair(const int clePair);
    int addPair(const char* clePair, const char* ip, const int port);
    int findNearbyElem(const int clePair, pair** ptr);

    /*******************************************************************
     * Fin des fonctions pour gérer les correspondances addrIP/cléPair.
     *******************************************************************/
    
    //Patterns pour utiliser les regex, afin d'extraire des données des messages.
    static const char PATTERN_IP_PORT[] = "([[:digit:]]{3} )?(([[:digit:]]{1,3}\\.){3}[[:digit:]]{1,3})/([[:digit:]]{2,6})()";
    static const char PATTERN_CLE_PAIR[] = "^([0-9]{3})? ?([0-9]+)()";

    char** extract(const char* msg, const char* pattern);
    int initClientConnect(int sdClient, struct sockaddr_in client_addr);

#ifdef	__cplusplus
}
#endif

#endif	/* CLIENTDATA_H */

