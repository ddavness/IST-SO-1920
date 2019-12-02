/*

    File: main.c
    Description: Main entry point for the project

    Group 84 (LEIC-T)
    Authors: David Duque (93698);
             Filipe Ferro (70611)

    Third project, Sistemas Operativos, IST/UL, 2019/20

*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <pthread.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/un.h>

#include "lib/color.h"
#include "lib/err.h"
#include "lib/inodes.h"
#include "lib/locks.h"
#include "lib/socket.h"
#include "lib/tecnicofs-api-constants.h"

#include "cmd.h"
#include "fs.h"

int numberBuckets = 0;
tecnicofs fs;

static void parseArgs (int argc, char** const argv){
    // For the nosync edition, we are allowing the two last arguments
    // (num_threads and num_buckets) to be omitted, since they're redundant
    if (argc != 4) {
        fprintf(stderr, red_bold("Invalid format!\n"));
        fprintf(stderr, red("Usage: %s %s %s %s\n"),
            argv[0],
            "socket_name",
            "output_file[.txt]",
            "num_buckets"
        );
        exit(EXIT_FAILURE);
    }

    // Validates the number of buckets
    numberBuckets = atoi(argv[3]);
    if (numberBuckets < 1) {
        fprintf(stderr, "%s\n%s %s\n",
            red_bold("Invalid number of buckets!"),
            red("Expected a positive integer, got"),
            argv[4]
        );
        exit(EXIT_FAILURE);
    } else {
        fprintf(stderr, green("Spawning %d buckets.\n\n"), numberBuckets);
    }
}

void deploy_threads(socket_t sock) {
    socket_t fork;
    while (true)
    {
        fork = acceptConnectionFrom(sock);
        printf("Connected!\nConnection details:\n %s %d\n %s %d\n",
            yellow_bold("> PID:"),
            fork.procId,
            yellow_bold("> UID:"),
            fork.userId
        );

        // TODO Store this somewhere
        socket_t* forkptr = malloc(sizeof(socket_t) + sizeof(tecnicofs));
        memcpy(forkptr, &fork, sizeof(socket_t));
        memcpy(forkptr + sizeof(socket_t), &fs, sizeof(tecnicofs));
        pthread_create(fork.thread, NULL, applyCommands, forkptr);
    }
}

int main(int argc, char** argv) {
    parseArgs(argc, argv);
    FILE* out;
    errWrap((out = fopen(argv[2], "w")) == NULL, "Unable to create/open output file!");

    inode_table_init();
    // Deploy our socket
    socket_t socket = newSocket(argv[1]);

    struct timeval start, end;

    // This time getting approach was found on https://stackoverflow.com/a/10192994
    fs = new_tecnicofs(numberBuckets);

    gettimeofday(&start, NULL);
    deploy_threads(socket);

    print_tecnicofs_tree(out, fs);
    fclose(out);

    free_tecnicofs(fs);
    inode_table_destroy();
    gettimeofday(&end, NULL);

    double elapsed = (((double)(end.tv_usec - start.tv_usec)) / 1000000.0) + ((double)(end.tv_sec - start.tv_sec));

    fprintf(stderr, green_bold("TecnicoFS completed in %.04f seconds.\n"), elapsed);
    exit(EXIT_SUCCESS);
}
