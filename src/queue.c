#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "queue.h"

void queue_init(queue_struct *q)
{
    q->head = NULL;
    q->size = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void queue_destroy(queue_struct *q)
{
    pthread_mutex_lock(&q->mutex);

    queue_node *curr = q->head;
    while (curr)
    {
        queue_node *tmp = curr;
        curr = curr->next;
        free(tmp);
    }

    q->head = NULL;
    q->size = 0;

    pthread_mutex_unlock(&q->mutex);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}

void queue_push(queue_struct *q, sha256_request *req, off_t filesize)
{
    queue_node *node = malloc(sizeof(queue_node));
    if (!node)
    {
        fprintf(stderr, "err: malloc failed in queue_push (%s)\n", strerror(errno));
        return;
    }

    node->req = *req; 
    node->filesize = filesize;
    node->next = NULL;

    pthread_mutex_lock(&q->mutex);

    if (!q->head || filesize < q->head->filesize)
    {
        node->next = q->head;
        q->head = node;
    }
    else
    {
        queue_node *curr = q->head;
        while (curr->next && curr->next->filesize <= filesize)
            curr = curr->next;
        node->next = curr->next;
        curr->next = node;
    }

    q->size++;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

int queue_pop(queue_struct *q, sha256_request *req, off_t *filesize)
{
    pthread_mutex_lock(&q->mutex);

    while (q->size == 0)
        pthread_cond_wait(&q->cond, &q->mutex);

    queue_node *node = q->head;
    q->head = node->next;
    q->size--;

    *req = node->req;
    *filesize = node->filesize;

    free(node);
    pthread_mutex_unlock(&q->mutex);
    return 1;
}

int queue_is_empty(queue_struct *q)
{
    pthread_mutex_lock(&q->mutex);
    int empty = (q->size == 0);
    pthread_mutex_unlock(&q->mutex);
    return empty;
}
