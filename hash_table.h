#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "item.h"

#define TARGET_ALPHA 5.0        // limit alpha, beyond it we do rehashing

typedef struct hash_table *HASH;

HASH hash_init(int maxN);
void hash_insert(HASH h, Item key, int id);
void resize_reverse_array(HASH h, int new_size);
int hash_search(HASH h, Item key);
void hash_remove(HASH h, int id);
void hash_free(HASH h);


#endif