#ifndef _JSON_OBJECTS_H
#define _JSON_OBJECTS_H

#define Node void *

#include <stdlib.h>
typedef struct vect{
    size_t size;
    size_t capacity;
    Node * data;
} Vector;

struct entries{
    char * key;
    size_t keylen;
    Node value;
    struct entries * next;
};

typedef struct map {
    size_t capacity;
    size_t size;
    struct entries ** buckets;
} Map;

size_t map_size(Map * map);
int map_remove(Map *map, const char * key, size_t length);
Map * map_create();
int map_insert(Map * map,  char * key, size_t len, Node value);
int map_get(Map * map,  char * key, size_t length, Node * value);
void map_free(Map * map);
size_t map_capacity(Map * map);

#endif
