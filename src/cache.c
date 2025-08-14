#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "cache.h"

void cache_init(cache_struct *cache)
{
    cache->head = NULL;
    pthread_mutex_init(&cache->mutex, NULL);
}

void cache_destroy(cache_struct *cache)
{
    pthread_mutex_lock(&cache->mutex);

    cache_node *curr = cache->head;
    while (curr)
    {
        cache_node *tmp = curr;
        curr = curr->next;
        pthread_mutex_destroy(&tmp->mutex);
        pthread_cond_destroy(&tmp->cond);
        free(tmp);
    }

    cache->head = NULL;

    pthread_mutex_unlock(&cache->mutex);
    pthread_mutex_destroy(&cache->mutex);
}

// find or insert entry for file path
cache_node *cache_lookup_or_insert(cache_struct *cache, const char *file_path, int *is_new)
{
    pthread_mutex_lock(&cache->mutex);

    cache_node *curr = cache->head;
    while (curr)
    {
        if (strcmp(curr->file_path, file_path) == 0)
        {
            *is_new = 0;
            pthread_mutex_unlock(&cache->mutex);
            return curr;
        }
        curr = curr->next;
    }

    cache_node *entry = malloc(sizeof(cache_node));
    if (!entry)
    {
        fprintf(stderr, "err: malloc failed (%s)\n", strerror(errno));
        pthread_mutex_unlock(&cache->mutex);
        return NULL;
    }

    strncpy(entry->file_path, file_path, sizeof(entry->file_path) - 1);
    entry->file_path[sizeof(entry->file_path) - 1] = '\0';
    entry->digest[0] = '\0';
    entry->ready = 0;
    pthread_mutex_init(&entry->mutex, NULL);
    pthread_cond_init(&entry->cond, NULL);

    entry->next = cache->head;
    cache->head = entry;

    *is_new = 1;

    pthread_mutex_unlock(&cache->mutex);
    return entry;
}

void cache_set_digest(cache_node *entry, const char *digest)
{
    pthread_mutex_lock(&entry->mutex);
    strncpy(entry->digest, digest, HASH_LEN);
    entry->digest[HASH_LEN - 1] = '\0';
    entry->ready = 1;
    pthread_cond_broadcast(&entry->cond);
    pthread_mutex_unlock(&entry->mutex);
}

int cache_get_digest(cache_struct *cache, const char *file_path, char *digest_out)
{
    pthread_mutex_lock(&cache->mutex);

    cache_node *curr = cache->head;
    while (curr)
    {
        if (strcmp(curr->file_path, file_path) == 0)
        {
            pthread_mutex_lock(&curr->mutex);

            if (curr->ready)
            {
                strncpy(digest_out, curr->digest, HASH_LEN);
                digest_out[HASH_LEN - 1] = '\0';
                pthread_mutex_unlock(&curr->mutex);
                pthread_mutex_unlock(&cache->mutex);
                return 1;
            }

            pthread_mutex_unlock(&curr->mutex);
            break;
        }
        curr = curr->next;
    }

    pthread_mutex_unlock(&cache->mutex);
    return 0;
}

void cache_print(cache_struct *cache)
{
    pthread_mutex_lock(&cache->mutex);
    printf("--- CACHE ---\n");

    cache_node *curr = cache->head;
    while (curr)
    {
        printf("%s : %s [%s]\n",
            curr->file_path,
            curr->digest,
            curr->ready ? "READY" : "PENDING");
        curr = curr->next;
    }

    printf("--------------\n");
    pthread_mutex_unlock(&cache->mutex);
}
