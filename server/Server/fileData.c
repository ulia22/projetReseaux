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

#define MAX_BUFFER_LENGHT 1024
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
 * Crée un nouveau fichier metadata à partir du contenu du socket sdClient jusqu'a ce qu'il n'y est plus rien à lire.
 * @param idFile
 * @param sdClient
 * @return 
 */
int newMetaDataFile(int idFile, int sdClient){
    char pathToNewFile[256];
    FILE* metaFile;
    char buffer[MAX_BUFFER_LENGHT];
    int read;
    
    //Création du path
    //Première partie du path
    memset(pathToNewFile, 0, 256);
    strcat(pathToNewFile, "./");
    strcat(pathToNewFile, META_DATA_DIR);
    strcat(pathToNewFile, "/");
    
    //Deuxième partie du path
    sprintf(pathToNewFile, "%d", idFile);
    
    //Ouverture du fichier en écriture
    metaFile = fopen(pathToNewFile, "w");
    if(metaFile == NULL){
        perror("Erreur open newMetaDataFile.");
        pthread_exit(NULL);
    }
    
    //Lecture du stream socket;
    do{
        read = 0;
        memset(buffer, 0, MAX_BUFFER_LENGHT);
        recv(sdClient, buffer, MAX_BUFFER_LENGHT, 0);
        
        //Sauvegarde dans file
        fprintf(metaFile, "%s", buffer);
        read = strlen(buffer);
    }while(read == MAX_BUFFER_LENGHT - 1 );
    
    //femeture du fichier
    fclose(metaFile);
    
    return 0;
}

/**
 * Envoi le metaFile name sur le socket sdClient, avec auparavent le préfixe prefix.
 * @param dirName
 * @param parentDir
 * @return  0 si ok sinon -1
 */
int sendMetaFile(int sdClient, char *name, char *prefix){
    char path[256], buffer[MAX_BUFFER_LENGHT];
    FILE* file;
    int read, totalLenght;
    char *fileC;
    
    memset(path, 0, 256);
    sprintf(path, "./%s/%s", META_DATA_DIR, name);
    
    file = fopen(path, "r");
    
    //Calcul de la longueur du fichier.
    totalLenght = 0;
    totalLenght += strlen(prefix);
    do{
        read = 0;
        memset(buffer, 0, MAX_BUFFER_LENGHT);
        read = strlen(fgets(buffer, MAX_BUFFER_LENGHT , file));
        totalLenght += read;
                
    }while(read == (MAX_BUFFER_LENGHT - 1));
   totalLenght ++; // '\0'
    
    //Reservation de la mémoire
    fileC = malloc(sizeof(char) * totalLenght);
    
    strcat(fileC, prefix);
    
    //Sauvegarde du fichier dans un char* fileC
    rewind(file);
    do{
        read = 0;
        memset(buffer, 0, MAX_BUFFER_LENGHT);
        read = strlen(fgets(buffer, MAX_BUFFER_LENGHT , file));
        strcat(fileC, buffer);                
    }while(read == (MAX_BUFFER_LENGHT - 1));
    
    //envoi du fichier sur la socket
    send(sdClient, fileC, strlen(fileC), 0);
    return 0;
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
 * Regarde si un meta-data_file existe.
 * @param fileName
 * @return 1 si oui 0 si non
 */
int metaFileExist(char* fileName){
    char pathParent[256];
    
    memset(pathParent, 0, 256);
    sprintf(pathParent, "%s%s", "./", META_DATA_DIR);

    return (fileExist(fileName, pathParent));
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

/**
 * Crée une cleFile unique à partir du fichier LAST_CLE_FILE qui contient la dernière clé créée.
 * @return la dernière clé créer.
 */
int getNewCleFile(){
    FILE* fd;
    char buffer[10];
    memset(buffer, 0, 10);
    char path[256];
    memset(path,0,256);
    strcat(path, "./");
    strcat(path, PERSISTENT_DATA_DIR);
    strcat(path, "/");
    strcat(path, LAST_CLE_FILE);
    
    fd = fopen(path, "r");
    if(fd == NULL){
        perror("Open file getNewCleFile");
        exit(EXIT_FAILURE);
    }
    fgets(buffer, 10, fd);
    fclose(fd);
    int cle = atoi(buffer);
    ++cle;
    
    fd = fopen(path, "w");
    if(fd == NULL){
        perror("Open file getNewCleFile");
        exit(EXIT_FAILURE);
    }
    memset(buffer, 0, 10);
    sprintf(buffer, "%d", cle);
    fputs(buffer, fd);
    fclose(fd);
    return cle;
}