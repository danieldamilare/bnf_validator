#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"
#include "objects.h"
#include <stdbool.h>
#define DEBUG


static inline void map_init(Map * map){
    map->capacity = 0;
    map->size = 0;
    map->buckets = NULL;
}

Map * map_create(){
    Map * map = jsmalloc(sizeof (*map));
    map_init(map);
    return map;
}

static inline uint32_t hash(const uint8_t* key, size_t length) {
 uint32_t hash = 2166136261u;
 for (size_t i = 0; i < length; i++) {
 hash ^= key[i];
 hash *= 16777619;
 }
 return hash;
}
static inline int map_adjust(Map * map){
    #define INITIAL_SIZE 100

     size_t size = map->capacity == 0? INITIAL_SIZE :
         map->capacity << 1;

     if (size == map->capacity) size++;
     if (size < map->capacity) return -1; //overflow?
     struct entries ** data = jsmalloc(size * sizeof(struct entries *));
     if (!data) return -1;

     for (size_t i = 0; i < size; i++) 
         data[i] = NULL;

     for (size_t i = 0; i < map->capacity; i++){
         struct entries * p;
         for (p = map->buckets[i]; p != NULL;){
             size_t pos = hash((uint8_t *) p->key, p->keylen) % size;
             struct entries * temp = p->next;
             p->next = data[pos];
             data[pos]  = p;
             p = temp;
         }
     }
     if (map->buckets){
         jsfree(map->buckets);
     }
     map->capacity = size;
     map->buckets = data;
     return 0;
}

static inline struct entries * map_lookup(Map * map, char * key, size_t length){
    if (map->capacity == 0) return NULL;
    uint32_t idx = hash((const uint8_t *) key, length) % map->capacity;
    struct entries * p = map->buckets[idx];
    while (p != NULL){
        if (p->keylen == length &&
            memcmp(key, p->key, length) == 0) return p;
        p = p->next;
    }
    return NULL;
}

size_t map_capacity(Map * map){
    return map->capacity;
}
size_t map_size(Map * map){
    return map->size;
}

int map_insert(Map * map, char * key, size_t length, Node value){
    if (!map || !key || *key == '\0') return -1;
#define LOAD_FACTOR 0.75
    struct entries * entry;

    if ((entry = map_lookup(map, key, length))){
        json_free(&entry->value);
        entry->value = value;
        return 0;
    }

    if (map->capacity == 0 || map->size + 1 >= map->capacity * LOAD_FACTOR){
        map_adjust(map);
    }

    uint32_t idx = hash((const uint8_t *)key, length) % map->capacity;
    struct entries * node = jsmalloc(sizeof (*node));

    char * dup = jsmalloc(length + 1);
    memcpy(dup, key, length);
    dup[length] = 0;
    node->key = dup;
    node->keylen = length;
    node->value = value;
    node->next = map->buckets[idx];
    map->buckets[idx] = node;
    map->size++;

    return 0;
#undef LOAD_FACTOR
}

int map_get(Map * map, 
         char * key, size_t length,  Node * value){
    if (!map || map->capacity == 0 || !value || !key || *key == '\0') return -1;
    struct entries * entry = map_lookup(map, key, length);
    if (!entry) return -1;
    *value = entry->value;
    return 0;
}

bool map_contains(Map * map, char * key, int length){
    return map_lookup(map, key,length) != NULL;
}

int map_remove(Map * map, const char * key, size_t length){
    if (!map || !map->capacity ||
        !key || *key == '\0') return -1;
    uint32_t idx = hash((const uint8_t *)key, length) % map->capacity;
    struct entries * entry = map->buckets[idx], *prev = NULL;

    while (entry != NULL){
        if (entry->keylen == length && 
            memcmp(entry->key, key, length) == 0){
            if (!prev){
                map->buckets[idx] = entry->next;
            } else{
                prev->next = entry->next;
            }
            jsfree(entry->key);
            json_free(&entry->value);
            jsfree(entry);
            map->size--;
            return 0;
        }
        prev = entry;
        entry = entry->next;
    }
    return -1;
}

void map_free(Map * map){
    if (!map) return;
    if (map->buckets && map->capacity > 0){
        struct entries * entry, *next;
        for (size_t i = 0; i < map->capacity; i++){
            entry = map->buckets[i];
            while(entry != NULL){
                next = entry->next;
                jsfree(entry->key);
                json_free(&entry->value);
                jsfree(entry);
                entry = next;
            }
        }
        jsfree(map->buckets);
    }
    jsfree(map);
}
