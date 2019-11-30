/*

    File: main.c
    Description: Main entry point for the project

    Group 84 (LEIC-T)
    Authors: David Duque (93698);
             Filipe Ferro (70611)

    Third project, Sistemas Operativos, IST/UL, 2019/20

*/

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
#include "lib/locks.h"
#include "lib/socket.h"
#include "lib/tecnicofs-api-constants.h"

#include "fs.h"

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
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
        fprintf(stderr, "%s\n%s %s\n", red_bold("Invalid number of buckets!"), red("Expected a positive integer, got"), argv[4]);
        exit(EXIT_FAILURE);
    } else {
        fprintf(stderr, green("Spawning %d buckets.\n\n"), numberBuckets);
    }
}

void emitParseError(char* cmd){
    fprintf(stderr, "%s '%s'", yellow("Error! Invalid Command:"), cmd);
    fprintf(stderr, red_bold("\nParsing failure!\n"));
    exit(EXIT_FAILURE);
}

void* applyCommands(void* socket){
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    socket_t sock = *((socket_t*)socket);
    int statuscode[1];
    char command[MAX_INPUT_SIZE];
    command[0] = '\0';

    char token;
    char name[MAX_INPUT_SIZE];
    char targ[MAX_INPUT_SIZE];

    for (;;) {
        int success = read(sock.socket, command, MAX_INPUT_SIZE);

        // Sanity verification block
        errWrap(success < 0, "Error reading commands!");
        if (!success) {
            // Client unmounted
            printf("Client hung up, exiting...\n");
            errWrap(close(sock.socket), "Unable to close socket fdescriptor!");
            pthread_exit(NULL);
            return NULL;
        }

        printf("'%s'\n", command);

        int numTokens = sscanf(command, "%c %s %s", &token, name, targ);

        if (numTokens != 3 && numTokens != 2) {
            fprintf(stderr, "%s '%s'\n", red("Caught invalid command:"), command);
            *statuscode = TECNICOFS_ERROR_OTHER;
            errWrap(send(sock.socket, statuscode, 1, 0) < 1, "Unable to deliver status code!");
            continue;
        }

        lock* fslock = get_lock(fs, name);
        lock* tglock;
        if (numTokens == 3) {
            tglock = get_lock(fs, targ);
        }
        int searchResult;
        int iNumber;
        switch (token) {
            case 'c':
                // We're now unlocking because we've got the iNumber!
                iNumber = obtainNewInumber(&fs);

                LOCK_WRITE(fslock);
                create(fs, name, iNumber);
                LOCK_UNLOCK(fslock);

                break;
            case 'l':
                LOCK_READ(fslock);
                searchResult = lookup(fs, name);

                if (!searchResult) {
                    printf("%s not found\n", name);
                } else {
                    printf("%s found with inumber %d\n", name, searchResult);
                }
                LOCK_UNLOCK(fslock);

                break;
            case 'd':
                LOCK_WRITE(fslock);
                delete(fs, name);
                LOCK_UNLOCK(fslock);

                break;
            case 'r':
                if (fslock == tglock) {
                    // We can simply rename in the tree
                    LOCK_WRITE(fslock);

                    int originFile = lookup(fs, name);
                    int targetFile = lookup(fs, targ);

                    if (originFile && !targetFile) {
                        delete(fs, name);
                        create(fs, targ, originFile);
                    }

                    LOCK_UNLOCK(fslock);
                } else {
                    if ((intptr_t)fslock > (intptr_t)tglock) {
                        // Swap the locks
                        lock* tmp = fslock;
                        fslock = tglock;
                        tglock = tmp;
                    }
                    // Lock both threes, delete on origin, create on target
                    LOCK_WRITE(fslock);
                    LOCK_WRITE(tglock);

                    int originFile = lookup(fs, name);
                    int targetFile = lookup(fs, targ);

                    if (originFile && !targetFile) {
                        delete(fs, name);
                        create(fs, targ, originFile);
                    }

                    LOCK_UNLOCK(tglock);
                    LOCK_UNLOCK(fslock);
                }

                break;
            default: {
                fprintf(stderr, "%s '%s'\n", red("Caught invalid command:"), command);
                *statuscode = TECNICOFS_ERROR_OTHER;
                errWrap(send(sock.socket, statuscode, 1, 0) < 1, "Unable to deliver status code!");
                break;
            }
        }
        *statuscode = TECNICOFS_OK;
        errWrap(send(sock.socket, statuscode, 1, 0) < 1, "Unable to deliver status code!");
    }

    return NULL;
}

void deploy_threads(socket_t sock) {
    while (true)
    {
        socket_t fork = acceptConnectionFrom(sock);
        printf("Connected!\n");
        pthread_create(fork.thread, NULL, applyCommands, &fork);
    }
}

int main(int argc, char** argv) {
    parseArgs(argc, argv);
    FILE* out = fopen(argv[2], "w");
    errWrap(out == NULL, "Unable to create/open output file!");

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
    gettimeofday(&end, NULL);

    double elapsed = (((double)(end.tv_usec - start.tv_usec)) / 1000000.0) + ((double)(end.tv_sec - start.tv_sec));

    fprintf(stderr, green_bold("TecnicoFS completed in %.04f seconds.\n"), elapsed);
    exit(EXIT_SUCCESS);
}
