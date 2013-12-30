#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "fileData.h"
#include "pair.h"

#define META_DATA_DIR "meta_data_dir"//Nom du dossier contenant la liste des meta-data des fichiers partagés.
//Tous les fichier meta-data se trouve dans le fichier ./.meta-data

//Doonées persistente
#define PERSISTENT_DATA_DIR "persist_data" //Emplacement du dossier pour la persistence.
#define LAST_CLE_PAIR "last_cle_pair"
#define LAST_CLE_FILE "last_cle_file"

#define INIT_CLE_PAIR "0"
#define INIT_CLE_FILE "0"

/**
 * 
 * @return 
 */
int initFileData() {
    //Initialisation du dossier contenant la liste des meta-data
    //On regarde si le dossier existe
    /*genererDIR(META_DATA_DIR, ".");

    //Création repertoire des données persistentes si il n'existe pas.
    genererDIR(PERSISTENT_DATA_DIR, ".");*/

    //Génère les fichiers lastClePair et lastCleFichier si ils n'existent pas.
    char clePairPath[256] = "./";
    strcat(clePairPath, PERSISTENT_DATA_DIR);

    char cleFilePath[256] = "./";
    strcat(cleFilePath, PERSISTENT_DATA_DIR);

    int err;
    if (!fileExist(LAST_CLE_PAIR, clePairPath)) {
        strcat(clePairPath, "/");
        strcat(clePairPath, LAST_CLE_PAIR);

        err = open(clePairPath, O_CREAT|O_RDWR|O_SYNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
        if(err == -1){
            perror("Open error\n");
            exit(EXIT_FAILURE);
        }
        close(err);
        
        FILE* file = fopen(clePairPath, "a+");
        if (file == NULL) {
            perror("Fopen erreur.\n");
            exit(EXIT_FAILURE);
        }

        fseek(file, 0, SEEK_SET);
        fputs(INIT_CLE_PAIR, file);
        fclose(file);
    }

    if (!fileExist(LAST_CLE_FILE, cleFilePath)) {
        strcat(cleFilePath, "/");
        strcat(cleFilePath, LAST_CLE_FILE);

        err = open(cleFilePath, O_CREAT|O_RDWR|O_SYNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
        if(err == -1){
            perror("Open error\n");
            exit(EXIT_FAILURE);
        }
        close(err);
        
        FILE* file = fopen(cleFilePath, "a+");
        if (file == NULL) {
            perror("Fopen erreur.\n");
            exit(EXIT_FAILURE);
        }

        fseek(file, 0, SEEK_SET);
        fputs(INIT_CLE_FILE, file);
        fclose(file);
    }
}

/**
 * Génère le dossier dirName dans le dossier parentDir si il n'existe pas.
 * @param dirName nom du dossier à générer si il n'existe pas.
 * @param parentDir dossier parent du dossier à générer.
 */
int genererDIR(const char* dirName, const char* parentDir) {
    //Initialisation du dossier contenant la liste des meta-data
    //On regarde si le dossier existe
    
    umask(~(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));
        struct stat buf;
    //memset(stat,0, sizeof(buf));
    struct dirent *lecture;
    DIR *rep;
    int exist = 0;

    rep = opendir(parentDir);
    while ((lecture = readdir(rep))) {
        if (strcmp(lecture->d_name, dirName) == 0 && lecture->d_type == DT_DIR) {
            exist = 1;
        }
    }
    if (!exist) {//Si le dossier n'existe pas, on le crée.
        char path[256];
        memset(path, '\0', 256);
        strcat(path, parentDir);
        strcat(path, "/");
        strcat(path, dirName);
        if (mkdir(path, 0666) == -1) {
            perror("Erreur création du repertoire des meta-data.");
            exit(EXIT_FAILURE);
        }
         stat(path, &buf);
        printf("Permissions:%d\n", buf.st_mode);
    }
    closedir(rep);
   
    
}

/**
 * Regarde si le fichier fileName esiste dans le dossier pathParent.
 * @return retourne 1 si il existe 0 sinon.
 */
int fileExist(char* fileName, char* pathParent) {
    struct dirent *lecture;
    DIR *rep;
    int exist = 0;

    rep = opendir(pathParent);
    while ((lecture = readdir(rep))) {
        if (strcmp(lecture->d_name, fileName) == 0 && lecture->d_type != DT_DIR) {
            exist = 1;
        }
    }
    closedir(rep);
    return exist;
}


/**
 * Cré une clePair unique à partir du fichier LAST_CLE_PAIR qui contient la dernière clé créée.
 * @return 
 */
int getNewClePair(void){
    FILE* fd;
    char buffer[10];
    memset(buffer, 0, 10);
    char path[256];
    memset(path,0,256);
    strcat(path, "./");
    strcat(path, PERSISTENT_DATA_DIR);
    strcat(path, "/");
    strcat(path, LAST_CLE_PAIR);
    
    fd = fopen(path, "r");
    if(fd == NULL){
        perror("Open file getNewClePair");
        exit(EXIT_FAILURE);
    }
    fgets(buffer, 10, fd);
    fclose(fd);
    int cle = atoi(buffer);
    ++cle;
    
    fd = fopen(path, "w");
    if(fd == NULL){
        perror("Open file getNewClePair");
        exit(EXIT_FAILURE);
    }
    memset(buffer, 0, 10);
    sprintf(buffer, "%d", cle);
    fputs(buffer, fd);
    fclose(fd);
    return cle;
}