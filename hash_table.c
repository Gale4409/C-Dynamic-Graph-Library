#include<stdio.h>
#include<stdlib.h>
#include "hash_table.h"
#include <string.h>


typedef struct hash_node *link;

struct hash_node{
    Item val;
    int id;
    link next;
};

struct hash_table{
    int M;      // size
    int currentN;       // number of the current elements in the ST
    int size_rev_array;     // always equal to V
    link *table;                // hash_table made with linearchaining O(1+α) time acces
    Item *reverse_array;        // used to know the item in the node id in the graph O(1) time access
};

int hash_func(Item item, int M){
    char *p = item.key;
    unsigned int  val = 0;
    for (; *p != '\0'; p++){
        val = 31 * val + *p;        // standard hash function for good distribuition
    }
    return val % M;
}

static link new_node(Item val, int id, link next){
    link x = malloc(sizeof(struct hash_node));
    if (x == NULL) return NULL;
    x->id = id; x->val = val; x->next = next;
    return x;
}

static link insert_node_head (Item val, int id, link h){
    if (h == NULL) return new_node(val, id, NULL);

    return new_node(val, id, h);
}

// allocating the hash_table
HASH hash_init(int maxN){
    HASH h = malloc(sizeof(struct hash_table));
    if (h == NULL) return NULL;
    h->M = maxN;
    h->currentN = 0;
    h->table = malloc(maxN*sizeof(link));
    for (int i = 0; i < maxN; i++){
        h->table[i] = NULL;
    }
    h->size_rev_array = maxN;
    h->reverse_array = malloc(maxN*sizeof(Item));
    return h;
}

// called by DynaGraphInsertNode if it has to realloc, we have to make space for new ids coming
void resize_reverse_array(HASH h, int new_size){
    h->size_rev_array = new_size;
    h->reverse_array = realloc(h->reverse_array, new_size*sizeof(Item));
}


// the new size would be double, no need to have a prime number because the hash_func already grants satisfacotry equiprobability
static void resize_hash_table(HASH h){
    link *new_table = malloc(2*h->M*sizeof(*new_table));    // preparing a new table of heads

    if (!new_table) return;

    int i; link t;

    for (i = 0; i < 2*h->M; i++){
        new_table[i] = NULL;
    }

    int new_index;
    // rehashing all the old items
    for (i = 0; i < h->M; i++){
        t = h->table[i];
        while (t != NULL){
            link next = t->next;
            new_index = hash_func(t->val, 2*h->M);
            t->next = new_table[new_index];         // saving the old head
            new_table[new_index] = t;           // putting the new node in head
            t = next;
        }
    }

    link *tmp = h->table;
    h->table = new_table;
    free(tmp);          // liberi solo il vecchio array di teste, non i nodi
    h->M *= 2;
}

// used to get the ID from the Item
int hash_search(HASH h, Item item){
    int index = hash_func(item, h->M);
    link t;
    for (t = h->table[index]; t != NULL; t = t->next){
        if (item_compare(t->val, item) == 0) return t->id;
    }
    return -1;
}


// Insert the new Item and its graph_id in the hash table
void hash_insert(HASH h, Item item, int id){
    if (hash_search(h, item) != -1) return;         // item already in the hash table


    int index = hash_func(item, h->M);
    h->table[index] = insert_node_head(item, id, h->table[index]);

    h->reverse_array[id] = item;
    h->currentN ++;

    // evaluating the resizing and rehashing
    if ((float) h->currentN / (float) h->M > TARGET_ALPHA){
        resize_hash_table(h);
    }
    return;
}

// FREE FUNCTION
static  void free_node(link t){
    free(t->val.key);
    free(t);
}

// remove the target node based on id comparison, O(1) check, better than Item_compare
static link remove_target_node(int id, link head){
    if (head == NULL) return NULL;

    link t = head, p = NULL;
    while (t != NULL){
        if (t->id == id){
            if (p == NULL){
                link tmp = t->next;
                free_node(t);
                return tmp;
            }
            else{
                p->next = t->next;
                free_node(t);
                return head;
            }
        }
        p = t;
        t = t->next;
    }
    return head;
}

void hash_remove(HASH h, int id){
    Item item_to_remove = h->reverse_array[id];

    if (item_to_remove.key == NULL) return;     // the id is already empty, this shouldn't happen

    int index = hash_func(item_to_remove, h->M);
    h->table[index] = remove_target_node(id, h->table[index]);
    h->reverse_array[id] = Item_set_void();     // marking the id as empty
    h->currentN --;
}

static void free_table(link *table, int M){
    int i;
    link t, tmp;
    for (i = 0; i < M; i++){
        for (t = table[i]; t != NULL;){
            tmp = t->next;
            free_node(t);
            t = tmp;
        }
    }
    free(table);
}

// still to decide who has the responsability of freeing the Items, at the moment this func does it
void hash_free(HASH h){
    free_table(h->table, h->M);
    for (int i = 0; i < h->size_rev_array; i++){
        h->reverse_array[i].key = NULL;
    }
    free(h->reverse_array);
    free(h);
}