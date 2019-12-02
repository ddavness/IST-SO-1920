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

#define RETURN_STATUS(STATUS) *statuscode = STATUS; errWrap(send(sock.socket, statuscode, sizeof(int), 0) < 1, "Unable to deliver status code!"); continue

void* applyCommands(void* socket){
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    socket_t sock = *((socket_t*)socket);
    int statuscode[1];
    char command[MAX_INPUT_SIZE];
    char arg1[MAX_INPUT_SIZE];
    char arg2[MAX_INPUT_SIZE];
    char token;

    for (;;) {
        // Shallow-clean buffers
        command[0] = '\0';
        arg1[0] = '\0';
        arg2[0] = '\0';

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

        int numTokens = sscanf(command, "%c %s %s", &token, arg1, arg2);

        if (numTokens != 3 && numTokens != 2) {
            RETURN_STATUS(TECNICOFS_ERROR_INVALID_SYNTAX);
        }

        int iNumber;
        switch (token) {
            case 'c': // creates a file (c filename perms)
            {
                // General syntax validation
                if (numTokens != 3) {
                    RETURN_STATUS(TECNICOFS_ERROR_INVALID_SYNTAX);
                }
                permission me = arg2[0] - '0';
                permission others = arg2[1] - '0';
                if (
                    me < 0 || me > 3 ||
                    others < 0 || others > 3 ||
                    arg2[2] != '\0'
                ) {
                    RETURN_STATUS(TECNICOFS_ERROR_INVALID_SYNTAX);
                }

                lock* fslock = get_lock(fs, arg1);
                LOCK_WRITE(fslock);

                // Does the file exist already?
                if (lookup(fs, arg1) >= 0) {
                    printf("'%s' already exists.\n", arg1);
                    LOCK_UNLOCK(fslock);;
                    RETURN_STATUS(TECNICOFS_ERROR_FILE_ALREADY_EXISTS);
                }

                // Get our iNumber
                iNumber = inode_create(sock.userId, me, others);
                if (iNumber < 0) {
                    // iNode table is full
                    LOCK_UNLOCK(fslock);
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                }

                // All checks passed, insert the file in the filesystem
                create(fs, arg1, iNumber);
                LOCK_UNLOCK(fslock);

                break;
            }
            case 'd': // delete file (d filename)
            {
                // General syntax validation
                if (numTokens != 2) {
                    RETURN_STATUS(TECNICOFS_ERROR_INVALID_SYNTAX);
                }

                lock* fslock = get_lock(fs, arg1);
                LOCK_WRITE(fslock);

                // Make sure the file does exist
                iNumber = lookup(fs, arg1);
                if (iNumber < 0) {
                    LOCK_UNLOCK(fslock);
                    RETURN_STATUS(TECNICOFS_ERROR_FILE_NOT_FOUND);
                }

                // Make sure the we are the actual owner of the file
                uid_t owner;
                if (inode_get(iNumber, &owner, NULL, NULL, NULL, 0) < 0) {
                    LOCK_UNLOCK(fslock);
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                }
                if (owner != sock.userId) {
                    LOCK_UNLOCK(fslock);
                    RETURN_STATUS(TECNICOFS_ERROR_PERMISSION_DENIED);
                }

                // All checks passed, delete the file

                inode_delete(iNumber);
                delete(fs, arg1);

                LOCK_UNLOCK(fslock);
                break;
            }
            case 'r': // rename file (r old new)
            {
                if (numTokens != 3) {
                    RETURN_STATUS(TECNICOFS_ERROR_INVALID_SYNTAX);
                }

                lock* fslock = get_lock(fs, arg1);
                lock* tglock = get_lock(fs, arg2);

                if (fslock == tglock) {
                    // Both names point to the same bucket
                    LOCK_WRITE(fslock);

                    // Make sure the file we're moving exists
                    iNumber = lookup(fs, arg1);
                    if (iNumber < 0) {
                        LOCK_UNLOCK(fslock);
                        RETURN_STATUS(TECNICOFS_ERROR_FILE_NOT_FOUND);
                    }

                    // Make sure we own the file we're moving
                    uid_t owner;
                    if (inode_get(iNumber, &owner, NULL, NULL, NULL, 0) < 0) {
                        LOCK_UNLOCK(fslock);
                        RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                    }
                    if (owner != sock.userId) {
                        LOCK_UNLOCK(fslock);
                        RETURN_STATUS(TECNICOFS_ERROR_PERMISSION_DENIED);
                    }
                    int targetFileiNumber = lookup(fs, arg2);

                    if (targetFileiNumber < 0) {
                        // Grant the rename
                        delete(fs, arg1);
                        create(fs, arg2, iNumber);
                    } else {
                        // The name we want is taken
                        LOCK_UNLOCK(fslock);
                        RETURN_STATUS(TECNICOFS_ERROR_FILE_ALREADY_EXISTS);
                    }

                    LOCK_UNLOCK(fslock);
                } else {
                    if ((intptr_t)fslock > (intptr_t)tglock) {
                        // Swap the locks
                        lock* tmp = fslock;
                        fslock = tglock;
                        tglock = tmp;
                    }
                    // Do the same steps as above except with both locks
                    // Lock both threes, delete on origin, create on target
                    LOCK_WRITE(fslock);
                    LOCK_WRITE(tglock);

                    iNumber = lookup(fs, arg1);
                    if (iNumber < 0) {
                        LOCK_UNLOCK(tglock);
                        LOCK_UNLOCK(fslock);
                        RETURN_STATUS(TECNICOFS_ERROR_FILE_NOT_FOUND);
                    }
                    uid_t owner;
                    if (inode_get(iNumber, &owner, NULL, NULL, NULL, 0) < 0) {
                        LOCK_UNLOCK(tglock);
                        LOCK_UNLOCK(fslock);
                        RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                    }
                    if (owner != sock.userId) {
                        LOCK_UNLOCK(tglock);
                        LOCK_UNLOCK(fslock);
                        RETURN_STATUS(TECNICOFS_ERROR_PERMISSION_DENIED);
                    }
                    int targetFile = lookup(fs, arg2);

                    if (targetFile < 0) {
                        delete(fs, arg1);
                        create(fs, arg2, iNumber);
                    } else {
                        LOCK_UNLOCK(tglock);
                        LOCK_UNLOCK(fslock);
                        RETURN_STATUS(TECNICOFS_ERROR_FILE_ALREADY_EXISTS);
                    }

                    LOCK_UNLOCK(tglock);
                    LOCK_UNLOCK(fslock);
                }

                break;
            }
            case 'o': // reads from an open file (o filename mode)
            {
                // General syntax validation
                if (numTokens != 3) {
                    RETURN_STATUS(TECNICOFS_ERROR_INVALID_SYNTAX);
                }

                permission mode = arg2[0] - '0';
                if (mode < 1 || mode > 3 || arg2[1] != '\0') {
                    RETURN_STATUS(TECNICOFS_ERROR_INVALID_SYNTAX);
                }

                lock* fslock = get_lock(fs, arg1);
                LOCK_READ(fslock);

                // Does the file we want to open actually exist?
                iNumber = lookup(fs, arg1);
                if (iNumber < 0) {
                    LOCK_UNLOCK(fslock);
                    RETURN_STATUS(TECNICOFS_ERROR_FILE_NOT_FOUND);
                }

                // Have we got the permissions required to open the file?
                uid_t owner;
                permission ownerPerms;
                permission generalPerms;

                if (inode_get(iNumber, &owner, &ownerPerms, &generalPerms, NULL, 0) < 0) {
                    LOCK_UNLOCK(fslock);
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                }
                if (sock.userId != owner) {
                    // Apply general permissions
                    if (!(mode & generalPerms)) {
                        LOCK_UNLOCK(fslock);
                        RETURN_STATUS(TECNICOFS_ERROR_PERMISSION_DENIED);
                    }
                } else {
                    // Apply owner permissions
                    if (!(mode & ownerPerms)) {
                        LOCK_UNLOCK(fslock);
                        RETURN_STATUS(TECNICOFS_ERROR_PERMISSION_DENIED);
                    }
                }

                // All checks passed, grant the file descriptor

                LOCK_UNLOCK(fslock);

                if (owner)

                break;
            }
            default: {
                RETURN_STATUS(TECNICOFS_ERROR_INVALID_SYNTAX);
                break;
            }
        }

        // Everything went smooth, return OK status
        RETURN_STATUS(TECNICOFS_OK);
    }

    return NULL;
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
        socket_t* forkptr = malloc(sizeof(socket_t));
        memcpy(forkptr, &fork, sizeof(socket_t));
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
