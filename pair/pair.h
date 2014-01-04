typedef struct Pair Pair;
struct Pair {
    char* data;
    Pair *suiv;
};

typedef struct ListePair ListePair;
struct ListePair {
    Pair *prem;
};

typedef struct ParamPartition ParamPartition;
struct ParamPartition {
    struct sockaddr_in add;
    char* cleFichier;
};
