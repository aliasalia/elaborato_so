#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "../include/common.h"

#define REQUEST_FIFO "../tmp/fifo_request"

int main(int argc, char *argv[])
{
    printf("[CLIENT] Hi, I'm the client!\n");
    if (argc < 2)
    {
        printf("Usage: %s <file_path>\n", argv[0]);
        return 1;
    }

    char response_fifo[256];
    snprintf(response_fifo, 256, FIFO_RESPONSE_TEMPLATE "%d", getpid());
    unlink(response_fifo);
    if (mkfifo(response_fifo, 0666) == -1 && errno != EEXIST)
    {
        perror("mkfifo resp");
        exit(1);
    }

    sha256_request req;
    req.pid = getpid();
    strncpy(req.file_path, argv[1], 256);

    // open req
    int req_fd = open(REQUEST_FIFO, O_WRONLY);
    if (req_fd == -1)
    {
        perror("open FIFO req");
        unlink(response_fifo);
        return 1;
    }
    write(req_fd, &req, sizeof(req));
    close(req_fd);

    printf("file: %s\n", response_fifo);

    // read res
    int res_fd = open(response_fifo, O_RDONLY);
    if (res_fd < 0)
    {
        perror("open response fifo");
        unlink(response_fifo);
        return 1;
    }

    sha256_response res;
    ssize_t n = read(res_fd, &res, sizeof(res));
    if (n == sizeof(res))
    {
        if (res.status == 0 || res.status == 3)
        {
            printf("SHA-256: %s\n", res.digest);
            if (res.status == 3)
                printf("[CLIENT] (cache answered)\n");
        }
        else
            printf("err\n");
    }
    else
        printf("err in res retrieve\n");

    close(res_fd);
    unlink(response_fifo);
    return 0;
}