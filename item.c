#include "item.h"
#include <stdio.h>
#include <string.h>

int item_compare(Item a, Item b){
    return strcmp(a.key, b.key);
}

Item Item_set_void(){
    Item empty;
    empty.key = NULL;
    return empty;
}