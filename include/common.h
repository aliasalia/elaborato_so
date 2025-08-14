#pragma once

#include <sys/types.h>
#define HASH_LEN 65
#define FIFO_RESPONSE_TEMPLATE "../tmp/fifo_response_"
#define CMD_EXIT "__EXIT__"

typedef struct {
    pid_t pid;
    char file_path[256];
} sha256_request;

typedef struct {
    char digest[HASH_LEN];
    int status; // 2=miss, 3=hit
} sha256_response;
