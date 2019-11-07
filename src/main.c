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
int numberCommands = 0;

int headQueue = 0;
int feedQueue = 0;
sem_t cmdFeed, cmdBuff;
pthread_mutex_t cmdlock, feedlock;

int numberThreads = 0;
int numberBuckets = 0;

tecnicofs fs;

static void parseArgs (int argc, char** const argv){
    // For the nosync edition, we are allowing the two last arguments
    // (num_threads and num_buckets) to be omitted, since they're redundant

    if ((NOSYNC && (argc > 5 || argc < 3)) || (!NOSYNC && argc != 5)) {
        fprintf(stderr, red_bold("Invalid format!\n"));
        fprintf(stderr, red("Usage: %s %s %s %s %s\n"), argv[0], "input_file[.txt]", "output_file[.txt]", "num_threads", "num_buckets");
        exit(EXIT_FAILURE);
    }

    if (NOSYNC) {
        // Display a warn if a thread/bucket argument was passed (and it is different than 1)
        if (
            (argc == 4 && (argv[3][0] != '1' || argv[3][1] != '\0')) ||
            (argc == 5 && (argv[3][0] != '1' || argv[3][1] != '\0' || argv[4][0] != '1' || argv[4][1] != '\0'))
        ){
            fprintf(stderr, yellow_bold("This program is ran in no-sync mode (sequentially), which means that it only runs one thread and one bucket.\n"));
        }

        numberThreads = 1;
        numberBuckets = 1;
    } else {
        // Validates the number of threads and buckets, if in MT mode

        int threads = atoi(argv[3]);
        int buckets = atoi(argv[4]);
        if (threads < 1) {
            fprintf(stderr, "%s\n%s %s\n", red_bold("Invalid number of threads!"), red("Expected a positive integer, got"), argv[3]);
            exit(EXIT_FAILURE);
        } else if (buckets < 1) {
            fprintf(stderr, "%s\n%s %s\n", red_bold("Invalid number of buckets!"), red("Expected a positive integer, got"), argv[4]);
            exit(EXIT_FAILURE);
        } else {
            fprintf(stderr, green("Spawning %d threads.\n\n"), threads);
            numberThreads = threads;
            numberBuckets = buckets;
        }
    }
}

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        numberCommands++;

        strcpy(inputCommands[feedQueue], data);
        feedQueue = (feedQueue + 1) % MAX_COMMANDS;
        return 0;
    }

    // This in shouldn't happen mid-execution if synchronization was done right
    // (Only in final cleanup, maybe)
    return 1;
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
        bool lineAdded = false;

        // Wait until the command can be replaced
        errWrap(sem_wait(&cmdFeed), "Error while waiting for the semaphore!");
        mutex_lock(&feedlock);

        char token;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s", &token, name);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
            case 'l':
            case 'd':
                if (numTokens != 2) {
                    break;
                }
                if (insertCommand(line)) {
                    // TODO: Panic, not too sure what to do here?...
                    fprintf(stderr, "Panic!\n");
                }
                lineAdded = true;
                break;
            case 'r':
                if (numTokens != 3){
                    emitParseError(line);
                    errs++;
                }
                if (insertCommand(line)) {
                    // TODO: Panic, not too sure what to do here?...
                    fprintf(stderr, "Panic!\n");
                }
                lineAdded = true;
                break;
            case '#':
                break;
            default: { /* error */
                emitParseError(line);
                errs++;
                break;
            }
        }

        mutex_unlock(&feedlock);
        // Signal that the command can be consumed
        if (lineAdded) {
            errWrap(sem_post(&cmdBuff), "Could not post on semaphore!");
        }
    }

    for (int i = 0; i < numberThreads; i++) {
        // Produce exit commands (defined as empty strings or null terminators)
        printf("Hi lol\n");

        // Wait until the command can be replaced
        errWrap(sem_wait(&cmdFeed), "Error while waiting for the semaphore!");
        mutex_lock(&feedlock);

        insertCommand("x");

        mutex_unlock(&feedlock);
        errWrap(sem_post(&cmdBuff), "Could not post on semaphore!");
        // Signal that the command can be consumed
    }
}

char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;

        char* cmd = inputCommands[headQueue];
        headQueue = (headQueue + 1) % MAX_COMMANDS;
        return cmd;
    }

    // This should NOT happen
    return NULL;
}

void* applyCommands(){
    char token;
    char name[MAX_INPUT_SIZE];
    char targ[MAX_INPUT_SIZE];

    while (true) {
        // Wait until a command can be consumed
        errWrap(sem_wait(&cmdBuff), "Error while waiting for the semaphore!");
        mutex_lock(&cmdlock);

        const char* command = removeCommand();
        if (command == NULL){
            fprintf(stderr, red_bold("Found null command in queue!\n"));
            exit(EXIT_FAILURE);
        } else if (command[0] == 'x') {
            mutex_unlock(&cmdlock);
            errWrap(sem_post(&cmdFeed), "Could not post on semaphore!");
            /* Allow the command to be replaced */

            return NULL;
        } else if (command[0] != 'c' && command[1] == ' ') {
            // Unlock early if the command doesn't require a new iNumber!
            mutex_unlock(&cmdlock);
            errWrap(sem_post(&cmdFeed), "Could not post on semaphore!");
        }

        int numTokens = sscanf(command, "%c %s %s", &token, name, targ);
        if (numTokens != 3 && numTokens != 2) {
            fprintf(stderr, "%s '%s'\n", red_bold("Error! Invalid command in Queue:"), command);
            exit(EXIT_FAILURE);
        }

        lock* fslock = get_lock(fs, name);
        int searchResult;
        int iNumber;
        switch (token) {
            case 'c':
                // We're now unlocking because we've got the iNumber!
                iNumber = obtainNewInumber(&fs);
                mutex_unlock(&cmdlock);
                errWrap(sem_post(&cmdFeed), "Could not post on semaphore!");

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
                break; // Silent Error 501 Not Implemented (yet)
            default: {
                fprintf(stderr, "%s %s\n", red_bold("Error! Invalid command in Queue:"), command);
                exit(EXIT_FAILURE);
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
