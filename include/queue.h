#pragma once

#include <pthread.h>
#include "common.h"

typedef struct queue_node {
    sha256_request req;
    off_t filesize;
    struct queue_node *next;
} queue_node;

typedef struct {
    queue_node *head;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int size;
} queue_struct;

void queue_init(queue_struct *q);
void queue_destroy(queue_struct *q);
void queue_push(queue_struct *q, sha256_request *req, off_t filesize);
int queue_pop(queue_struct *q, sha256_request *req, off_t *filesize);
int queue_is_empty(queue_struct *q);
