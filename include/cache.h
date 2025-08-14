#pragma once

#include <pthread.h>
#include "common.h"

typedef struct cache_node {
    char file_path[40];
    char digest[HASH_LEN];
    int ready; // 0= in corso, 1=pronto
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    struct cache_node *next;
} cache_node;

typedef struct {
    cache_node *head;
    pthread_mutex_t mutex;
} cache_struct;

void cache_init(cache_struct *cache);
void cache_destroy(cache_struct *cache);
cache_node *cache_lookup_or_insert(cache_struct *cache, const char *file_path, int *is_new);
void cache_set_digest(cache_node *entry, const char *digest);
int cache_get_digest(cache_struct *cache, const char *file_path, char *digest_out);
void cache_print(cache_struct *cache);
