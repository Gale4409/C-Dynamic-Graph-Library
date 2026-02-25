#include <stdio.h>
#include <stdlib.h>
#include "UF.h"

struct uf{
    int N;
    int *id;
    int *size;
};

UF UF_init(int N){
    UF uf = malloc(sizeof(struct uf));
    uf->N = N;
    uf->size = malloc(N*sizeof(int));
    uf->id = malloc(N*sizeof(int));
    for (int i = 0; i < N; i++){
        uf->id[i] = i; uf->size[i] = 1;
    }
    return uf;
}

int UF_find(UF uf, int p){
    int i;
    for (i = p; i != uf->id[i]; i = uf->id[i]);
    return i ;
}

// it's not a void because it returns -1 if the union is not successful, 0 otherwise
int UF_union(UF uf, int p, int q){
    int i = UF_find(uf, p);
    int j = UF_find(uf, q);

    if(i == j) return -1;

    if (uf->size[i] > uf->size[j]) {
        uf->size[i] += uf->size[j];
        uf->id[j] = uf->id[i];
    }

    else {
        uf->size[j] += uf->size[i];
        uf->id[i] = uf->id[j];
    }
    return 0;
}

void UF_free(UF uf){
    free(uf->id);
    free(uf->size);
    free(uf);
}
