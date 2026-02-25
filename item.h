#ifndef ITEM_H
#define ITEM_H

typedef struct{
    char *key;
}Item;

int item_compare(Item a, Item b);
Item Item_set_void();

#endif