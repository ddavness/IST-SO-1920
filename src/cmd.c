/*

    File: cmd.c
    Description: Implements the main functionality of the server.

*/

#define _GNU_SOURCE

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "lib/err.h"
#include "lib/socket.h"
#include "lib/inodes.h"
#include "lib/tecnicofs-api-constants.h"

#include "fs.h"

#define RETURN_STATUS(STATUS) *statuscode = STATUS; errWrap(send(sock.socket, statuscode, sizeof(int), 0) < 1, "Unable to deliver status code!"); continue
#define MAX_INPUT_SIZE 1024

typedef struct fd {
    int inode;
    permission mode;
} filed;

void* applyCommands(void* args){
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    socket_t sock = *((socket_t*)args);
    tecnicofs fs = *((tecnicofs*)((intptr_t)args + (intptr_t)sizeof(socket_t)));

    int statuscode[1];
    filed openfiles[MAX_OPEN_FILES];

    char command[MAX_INPUT_SIZE];
    char arg1[MAX_INPUT_SIZE];
    char arg2[MAX_INPUT_SIZE];
    char token;

    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        openfiles[i].inode = -1;
    }

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
            RETURN_STATUS(TECNICOFS_ERROR_OTHER);
        }

        int iNumber;
        switch (token) {
            case 'c': // creates a file (c filename perms)
            {
                // General syntax validation
                if (numTokens != 3) {
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                }
                permission me = arg2[0] - '0';
                permission others = arg2[1] - '0';
                if (
                    me < 0 || me > 3 ||
                    others < 0 || others > 3 ||
                    arg2[2] != '\0'
                ) {
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
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
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
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
                // And that such file is not open

                uid_t owner;
                int fileIsOpen;
                if (inode_get(iNumber, &fileIsOpen, &owner, NULL, NULL, NULL, 0) < 0) {
                    LOCK_UNLOCK(fslock);
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                } else if (owner != sock.userId) {
                    LOCK_UNLOCK(fslock);
                    RETURN_STATUS(TECNICOFS_ERROR_PERMISSION_DENIED);
                } else if (fileIsOpen) {
                    LOCK_UNLOCK(fslock);
                    RETURN_STATUS(TECNICOFS_ERROR_FILE_IS_OPEN);
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
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
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
                    if (inode_get(iNumber, NULL, &owner, NULL, NULL, NULL, 0) < 0) {
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
                    if (inode_get(iNumber, NULL, &owner, NULL, NULL, NULL, 0) < 0) {
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
            case 'o': // opens a file (o filename mode)
            {
                // General syntax validation
                if (numTokens != 3) {
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                }

                permission mode = arg2[0] - '0';
                if (mode < 1 || mode > 3 || arg2[1] != '\0') {
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                }

                lock* fslock = get_lock(fs, arg1);
                LOCK_READ(fslock);

                // Does the file we want to open actually exist?
                iNumber = lookup(fs, arg1);
                if (iNumber < 0) {
                    LOCK_UNLOCK(fslock);
                    RETURN_STATUS(TECNICOFS_ERROR_FILE_NOT_FOUND);
                }

                // Make sure we don't have a file descriptor for this file already
                // and that we have room for one
                int freeSlot = -1;
                for (int i = 0; i < MAX_OPEN_FILES; i++) {
                    if (openfiles[i].inode == iNumber) {
                        LOCK_UNLOCK(fslock);
                        RETURN_STATUS(TECNICOFS_ERROR_FILE_IS_OPEN);
                    } else if (openfiles[i].inode < 0 && freeSlot < 0) {
                        freeSlot = i;
                    }
                }

                if (freeSlot < 0) {
                    LOCK_UNLOCK(fslock);
                    RETURN_STATUS(TECNICOFS_ERROR_MAXED_OPEN_FILES);
                }

                // Have we got the permissions required to open the file?
                uid_t owner;
                permission ownerPerms;
                permission generalPerms;

                if (inode_get(iNumber, NULL, &owner, &ownerPerms, &generalPerms, NULL, 0) < 0) {
                    LOCK_UNLOCK(fslock);
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                }
                if (sock.userId != owner) {
                    // Apply general permissions
                    if ((mode & generalPerms) != mode) {
                        LOCK_UNLOCK(fslock);
                        RETURN_STATUS(TECNICOFS_ERROR_PERMISSION_DENIED);
                    }
                } else {
                    // Apply owner permissions
                    if ((mode & ownerPerms) != mode) {
                        LOCK_UNLOCK(fslock);
                        RETURN_STATUS(TECNICOFS_ERROR_PERMISSION_DENIED);
                    }
                }

                // All checks passed, grant the file descriptor
                if (inode_update_fd(iNumber, 1) < 0) {
                    LOCK_UNLOCK(fslock);
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                }
                openfiles[freeSlot].inode = iNumber;
                openfiles[freeSlot].mode = mode;

                LOCK_UNLOCK(fslock);

                // Return the new fd
                RETURN_STATUS(freeSlot);

                break;
            }
            case 'x': // closes an open file (x fd)
            {
                // General syntax validation
                if (numTokens != 2) {
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                }

                int fd = atoi(arg1);
                if (fd < 0 || fd >= MAX_OPEN_FILES) {
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                } else if (openfiles[fd].inode < 0) {
                    // This filedescriptor wasn't linked to anything
                    RETURN_STATUS(TECNICOFS_ERROR_FILE_NOT_OPEN);
                }

                // Update the filedescriptors
                if (inode_update_fd(openfiles[fd].inode, -1) < 0) {
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                }
                openfiles[fd].inode = -1;

                break;
            }
            case 'l': // reads len bytes of a file (l fd len)
            {
                // General syntax validation
                if (numTokens != 3) {
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                }

                // Invalid arguments
                int fd = atoi(arg1);
                int len = atoi(arg2);
                if (fd < 0 || fd >= MAX_OPEN_FILES || len <= 0) {
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                }

                // Make sure our fd is valid
                filed f = openfiles[fd];
                if (f.inode < 0) {
                    RETURN_STATUS(TECNICOFS_ERROR_FILE_NOT_OPEN);
                }

                // Make sure our fd is open in a valid mode
                if (f.mode != READ && f.mode != RW) {
                    RETURN_STATUS(TECNICOFS_ERROR_INVALID_MODE);
                }

                // Create our buffer: [iiii|c|c|c|c|c|...|c|c]
                void* readBuffer = malloc(sizeof(int) + (len + 1) * sizeof(char));

                int* status = readBuffer;
                char* contents = (char*)((intptr_t)readBuffer + (intptr_t)sizeof(int));

                // Copy the file contents to the buffer
                int charsRead = inode_get(f.inode, NULL, NULL, NULL, NULL, contents, len);
                if (charsRead < 0) {
                    RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                }

                // Perform required adjustments to the buffer
                *status = charsRead;
                readBuffer = realloc(readBuffer, sizeof(int) + (charsRead + 1) * sizeof(char));
                ssize_t bufferSize = sizeof(int) + (charsRead + 1) * sizeof(char);

                // Manually send this over to the client and return
                errWrap(
                    send(sock.socket, readBuffer, bufferSize, 0) < bufferSize,
                    "Unable to send stuff over to client!"
                );
                continue;
                break;
            }
            case 'w': // writes the message supplied to file (w fd msg)
            {
                // General validation syntax
            }
            default: {
                RETURN_STATUS(TECNICOFS_ERROR_OTHER);
                break;
            }
        }

        // Everything went smooth, return OK status
        RETURN_STATUS(TECNICOFS_OK);
    }

    return NULL;

    #undef RETURN_STATUS
}