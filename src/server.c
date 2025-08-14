#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>
#include <openssl/sha.h>
#include <signal.h>
#include <stdatomic.h>
#include "../include/queue.h"
#include "../include/cache.h"
#include "../include/common.h"

#define THREAD_POOL_SIZE 3
#define FIFO_NAME "../tmp/fifo_request"

queue_struct req_queue;
cache_struct cache;
int fd = -1;
atomic_int running = 1;

void handle_signal(int signum)
{
    printf("\n[SERVER] Closing signal recieved...\n");
    running = 0;
}

int digest_file(const char *file_path, char *out_digest)
{
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    char buffer[32];
    int file = open(file_path, O_RDONLY, 0);
    if (file == -1)
    {
        printf("File %s does not exist\n", file_path);
        return -1;
    }
    ssize_t bR;

    do
    {
        // read the file in chunks of 32 characters
        bR = read(file, buffer, 32);
        if (bR > 0)
            SHA256_Update(&ctx, buffer, bR);
        else if (bR < 0)
        {
            printf("File %s does not exist\n", file_path);
            close(file);
            return -1;
        }
    } while (bR > 0);

    unsigned char hash[32];
    SHA256_Final(hash, &ctx);
    close(file);

    for (int i = 0; i < 32; i++)
        sprintf(out_digest + (i * 2), "%02x", hash[i]);

    out_digest[64] = '\0';
    printf("Digest: %s\n", out_digest);
    return 0;
}

void *worker(void *arg)
{
    while (1)
    {
        sha256_request req;
        off_t filesize;

        queue_pop(&req_queue, &req, &filesize);

        if (strcmp(req.file_path, CMD_EXIT) == 0)
        {
            printf("[SERVER] Worker exiting...\n");
            break;
        }

        int new;
        cache_node *entry = cache_lookup_or_insert(&cache, req.file_path, &new);

        pthread_mutex_lock(&entry->mutex);
        if (!new)
        {
            while (!entry->ready)
                pthread_cond_wait(&entry->cond, &entry->mutex);

            pthread_mutex_unlock(&entry->mutex);
            char resp_fifo[256];
            snprintf(resp_fifo, sizeof(resp_fifo), FIFO_RESPONSE_TEMPLATE "%d", req.pid);
            int fd_resp = open(resp_fifo, O_WRONLY);
            if (fd_resp != -1)
            {
                sha256_response resp;
                strncpy(resp.digest, entry->digest, HASH_LEN);
                resp.status = 3; // hit
                write(fd_resp, &resp, sizeof(resp));
                close(fd_resp);
            }
            continue;
        }
        pthread_mutex_unlock(&entry->mutex);

        // new entry
        char char_hash[HASH_LEN];
        int status = digest_file(req.file_path, char_hash);
        // cache update
        cache_set_digest(entry, (status == 0) ? char_hash : "ERROR");

        // send resp
        char resp_fifo[256];
        snprintf(resp_fifo, sizeof(resp_fifo), FIFO_RESPONSE_TEMPLATE "%d", req.pid);
        int fd_resp = open(resp_fifo, O_WRONLY);
        if (fd_resp != -1)
        {
            sha256_response resp;
            strncpy(resp.digest, (status == 0) ? char_hash : "ERROR", HASH_LEN);
            resp.status = (status == 0) ? 0 : 1;
            write(fd_resp, &resp, sizeof(resp));
            close(fd_resp);
        }
    }
    return NULL;
}

int main()
{
    printf("[SERVER] Hi, I'm the server!\n");
    queue_init(&req_queue);
    cache_init(&cache);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // rm
    unlink(FIFO_NAME);
    // make FIFO
    if (mkfifo(FIFO_NAME, 0666) == -1 && errno != EEXIST)
    {
        perror("mkfifo req");
        exit(1);
    }
    int fd = open(FIFO_NAME, O_RDONLY | O_NONBLOCK);
    if (fd == -1)
    {
        perror("open FIFO req");
        exit(1);
    }

    pthread_t threads[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; ++i)
        pthread_create(&threads[i], NULL, worker, NULL);

    char buf[100];
    while (running)
    {
        sha256_request req;
        ssize_t n = read(fd, &req, sizeof(req));
        printf("n: %zu\n", n);
        if (n == sizeof(req))
        {
            off_t file_size = 0;

            struct stat st;
            if (stat(req.file_path, &st) == 0)
                file_size = st.st_size;
            else
                file_size = 0;

            queue_push(&req_queue, &req, file_size);
        }
        else
            usleep(1000000);
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        sha256_request exit_req;
        memset(&exit_req, 0, sizeof(exit_req));
        strcpy(exit_req.file_path, CMD_EXIT);
        queue_push(&req_queue, &exit_req, 0);
    }

    // Aspetta la terminazione dei worker
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
        pthread_join(threads[i], NULL);

    close(fd);
    queue_destroy(&req_queue);
    cache_destroy(&cache);
    unlink(FIFO_NAME);

    printf("[SERVER] Closed\n");
    return 0;
}