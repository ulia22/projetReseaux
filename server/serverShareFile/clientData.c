#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define META_DATA_DIR ".meta_data_dir"//Nom du dossier contenant la liste des meta-data des fichiers partagés.

struct linkIpKey {
    int key;//Clé unique du client
    char ipAddr[16]; //Adresse ip du client
};


int initClientData(){
    
   //Initialisation du dossier contenant la liste des meta-data
    //On regarde si le dossier existe
    struct dirent *lecture;
    DIR *rep;
    int exist = 0;
    char path[255] = {'\0'};
    rep = opendir("." );
    while ((lecture = readdir(rep))) {
        if(strcmp(lecture->d_name, META_DATA_DIR) == 0 && lecture->d_type == DT_DIR){
         exist = 1;   
        }
    }
    
    strcat(path, "./");
    strcat(path, META_DATA_DIR);
    if(exist != 0){//Si le dossier n'existe pas, on le crée.
        if(mkdir(path, 0666) == -1){
            perror("Erreur création du repertoire des meta-data");
            exit(EXIT_FAILURE);
        }
    }
    closedir(rep);
    
    //Regard initialisation des paires 
    
    
    
}
