#ifndef UF_H
#define UF_H

typedef struct uf *UF;

UF UF_init (int N);
void UF_free (UF uf);
int UF_find (UF uf, int p);
int UF_union (UF uf, int i, int j);


#endif