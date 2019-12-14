/*

    File: main.c
    Description: Main entry point for the project

    Group 84 (LEIC-T)
    Authors: David Duque (93698);
             Filipe Ferro (70611)

    First project, Sistemas Operativos, IST/UL, 2019/20

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
#include <semaphore.h>
#include <sys/types.h>
#include <sys/time.h>

#include "lib/color.h"
#include "lib/err.h"
#include "lib/locks.h"
#include "lib/void.h"
#include "fs.h"

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];

int headQueue = 0;
int feedQueue = 0;
sem_t cmdFeed, cmdBuff;
pthread_mutex_t cmdlock;

int numberThreads = 0;
int numberBuckets = 0;

tecnicofs fs;

static void parseArgs (int argc, char** const argv){
    // For the nosync edition, we are allowing the two last arguments
    // (num_threads and num_buckets) to be omitted, since they're redundant

    if ((NOSYNC && (argc > 5 || argc < 4)) || (!NOSYNC && argc != 5)) {
        fprintf(stderr, red_bold("Invalid format!\n"));
        fprintf(stderr, red("Usage: %s %s %s %s %s\n"),
            argv[0],
            "input_file[.txt]",
            "output_file[.txt]",
            NOSYNC ? "[num_threads = 1]" : "num_threads",
            "num_buckets"
        );
        exit(EXIT_FAILURE);
    }

    if (NOSYNC) {
        // Display a warn if a thread/bucket argument was passed (and it is different than 1)
        char* buckets;

        if (argc == 5){
            if (argv[3][0] != '1' || argv[3][1] != '\0'){
                fprintf(stderr, yellow_bold("This program is ran in no-sync mode (sequentially), which means that it only runs one thread.\n"));
            }
            buckets = argv[4];
        } else {
            buckets = argv[3];
        }

        numberThreads = 1;
        numberBuckets = atoi(buckets);
        if (numberBuckets < 1) {
            fprintf(stderr, "%s\n%s %s\n", red_bold("Invalid number of buckets!"), red("Expected a positive integer, got"), buckets);
            exit(EXIT_FAILURE);
        }
    } else {
        // Validates the number of threads and buckets, if in MT mode

        numberThreads = atoi(argv[3]);
        numberBuckets = atoi(argv[4]);
        if (numberThreads < 1) {
            fprintf(stderr, "%s\n%s %s\n", red_bold("Invalid number of threads!"), red("Expected a positive integer, got"), argv[3]);
            exit(EXIT_FAILURE);
        } else if (numberBuckets < 1) {
            fprintf(stderr, "%s\n%s %s\n", red_bold("Invalid number of buckets!"), red("Expected a positive integer, got"), argv[4]);
            exit(EXIT_FAILURE);
        } else {
            fprintf(stderr, green("Spawning %d threads.\n\n"), numberThreads);
        }
    }
}

void insertCommand(char* data) {
    strcpy(inputCommands[feedQueue], data);
    feedQueue = (feedQueue + 1) % MAX_COMMANDS;
}

void emitParseError(char* cmd){
    fprintf(stderr, "%s '%s'", yellow("Error! Invalid Command:"), cmd);
    fprintf(stderr, red_bold("\nParsing failure!\n"));
    exit(EXIT_FAILURE);
}

void feedInput(FILE* input){
    char line[MAX_INPUT_SIZE];
    int errs = 0;

    while (fgets(line, sizeof(line)/sizeof(char), input)) {
        bool lineAdded = true;

        // Wait until the command can be replaced
        errWrap(sem_wait(&cmdFeed), "Error while waiting for the feeder semaphore!");

        char token;
        char name[MAX_INPUT_SIZE];
        char targ[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %s\n", &token, name, targ);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
            case 'l':
            case 'p':
            case 'd':
                if (numTokens == 2) {
                    insertCommand(line);
                    break;
                } else {
                    continue;
                }
            case 'r':
                if (numTokens != 3){
                    emitParseError(line);
                    errs++;
                }
                insertCommand(line);
                break;
            case '#':
                lineAdded = false;
                break;
            default: { /* error */
                emitParseError(line);
                errs++;
                break;
            }
        }

        // Signal that the command can be consumed
        if (lineAdded) {
            errWrap(sem_post(&cmdBuff), "Could not post on buffer semaphore!");
        }
    }

    for (int i = 0; i < numberThreads; i++) {
        // Produce exit commands (defined as the 'x' command)
        // Wait until the command can be replaced
        errWrap(sem_wait(&cmdFeed), "Error while waiting for the feeder semaphore!");

        insertCommand("x");

        errWrap(sem_post(&cmdBuff), "Could not post on buffer semaphore!");
        // Signal that the command can be consumed
    }
}

char* removeCommand() {
    char* cmd = inputCommands[headQueue];
    headQueue = (headQueue + 1) % MAX_COMMANDS;
    return cmd;
}

void* applyCommands(){
    char token;
    char name[MAX_INPUT_SIZE];
    char targ[MAX_INPUT_SIZE];

    while (true) {
        // Wait until a command can be consumed
        errWrap(sem_wait(&cmdBuff), "Error while waiting for the buffer semaphore!");
        mutex_lock(&cmdlock);

        const char* command = removeCommand();
        if (command == NULL){
            fprintf(stderr, red_bold("Found null command in queue!\n"));
            exit(EXIT_FAILURE);
        } else if (command[0] == 'x') {
            mutex_unlock(&cmdlock);
            errWrap(sem_post(&cmdFeed), "Could not post on feeder semaphore!");
            /* Allow the command to be replaced */

            return NULL;
        }

        int numTokens = sscanf(command, "%c %s %s", &token, name, targ);
        if (numTokens != 3 && numTokens != 2) {
            fprintf(stderr, "%s '%s'\n", red_bold("Error! Invalid command in Queue:"), command);
            exit(EXIT_FAILURE);
        }

        lock* fslock = get_lock(fs, name);
        lock* tglock;
        if (numTokens == 3) {
            tglock = get_lock(fs, targ);
        }
        int searchResult;
        int iNumber;
        switch (token) {
            case 'p':
                ;// Hold the lock for x seconds (currently it's already locked)

                int seconds = atoi(name);
                if (seconds <= 0) {
                    // Invalid argument smh
                    mutex_unlock(&cmdlock);
                    errWrap(sem_post(&cmdFeed), "Could not post on feeder semaphore!");
                    break; 
                }
                printf("Pausa de %d segundos.\n", seconds);
                sleep(seconds);

                // Finally unlock it
                mutex_unlock(&cmdlock);
                errWrap(sem_post(&cmdFeed), "Could not post on feeder semaphore!");

                break;
            case 'c':
                // We're now unlocking because we've got the iNumber!
                iNumber = obtainNewInumber(&fs);
                mutex_unlock(&cmdlock);
                errWrap(sem_post(&cmdFeed), "Could not post on feeder semaphore!");

                LOCK_WRITE(fslock);
                create(fs, name, iNumber);
                LOCK_UNLOCK(fslock);

                break;
            case 'l':
                mutex_unlock(&cmdlock);
                errWrap(sem_post(&cmdFeed), "Could not post on feeder semaphore!");

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
                mutex_unlock(&cmdlock);
                errWrap(sem_post(&cmdFeed), "Could not post on feeder semaphore!");

                LOCK_WRITE(fslock);
                delete(fs, name);
                LOCK_UNLOCK(fslock);

                break;
            case 'r':
                mutex_unlock(&cmdlock);
                errWrap(sem_post(&cmdFeed), "Could not post on feeder semaphore!");

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
                mutex_unlock(&cmdlock);
                errWrap(sem_post(&cmdFeed), "Could not post on feeder semaphore!");

                fprintf(stderr, "%s %s\n", red_bold("Error! Invalid command in Queue:"), command);
                exit(EXIT_FAILURE);
                break;
            }
        }
    }

    return NULL;
}

void deploy_threads(FILE* cmds) {
    // Initialize the IO lock and local thread pool.
    errWrap(sem_init(&cmdFeed, 0, MAX_COMMANDS), "Unable to initialize feeder semaphore!");
    errWrap(sem_init(&cmdBuff, 0, 0), "Unable to initialize buffer semaphore!");

    mutex_init(&cmdlock);
    pthread_t threadPool[numberThreads];

    for (int i = 0; i < numberThreads; i++) {
        errWrap(pthread_create(&threadPool[i], NULL, applyCommands, NULL), "Failed to spawn thread!");
    }

    // Carry on, feed the buffer
    feedInput(cmds);

    // Wait until every thread is done.
    for (int i = 0; i < numberThreads; i++) {
        errWrap(pthread_join(threadPool[i], NULL), "Failed to join thread!");
    }

    mutex_destroy(&cmdlock);
}

int main(int argc, char** argv) {
    parseArgs(argc, argv);

    // Try to open the input file.
    // Possible errors when opening the file: No such directory, Access denied
    FILE* cmds = fopen(argv[1], "r");
    if (!cmds) {
        fprintf(stderr, red_bold("Unable to open file '%s'!"), argv[1]);
        perror("\nError");
        exit(EXIT_FAILURE);
    }
    // Try to open the output file.
    // Possible errors when opening/creating the file: Access denied
    FILE* out = fopen(argv[2], "w");
    if (!out) {
        fprintf(stderr, red_bold("Unable to open file '%s'!"), argv[2]);
        perror("\nError");
        exit(EXIT_FAILURE);
    }

    struct timeval start, end;

    // This time getting approach was found on https://stackoverflow.com/a/10192994
    fs = new_tecnicofs(numberBuckets);

    gettimeofday(&start, NULL);
    deploy_threads(cmds);

    print_tecnicofs_tree(out, fs);
    fclose(out);
    fclose(cmds);

    free_tecnicofs(fs);
    gettimeofday(&end, NULL);

    double elapsed = (((double)(end.tv_usec - start.tv_usec)) / 1000000.0) + ((double)(end.tv_sec - start.tv_sec));

    fprintf(stderr, green_bold("TecnicoFS completed in %.04f seconds.\n"), elapsed);
    exit(EXIT_SUCCESS);
}
