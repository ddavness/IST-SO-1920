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
#include <sys/types.h>
#include <sys/time.h>

#include "lib/color.h"
#include "lib/locks.h"
#include "lib/void.h"
#include "fs.h"

#ifdef MUTEX
    // Map macros to mutex

    pthread_mutex_t CMD_LOCK;
    pthread_mutex_t FS_LOCK;
    #define INIT_LOCKS() mutex_init(&CMD_LOCK); mutex_init(&FS_LOCK)
    #define LOCK_READ mutex_lock
    #define LOCK_WRITE mutex_lock
    #define LOCK_UNLOCK mutex_unlock
    #define DESTROY_LOCKS() mutex_destroy(&CMD_LOCK); mutex_destroy(&FS_LOCK)

    #define NOSYNC false
#elif RWLOCK
    // Map macros to RWLOCK

    pthread_rwlock_t CMD_LOCK;
    pthread_rwlock_t FS_LOCK;
    #define INIT_LOCKS() rwlock_init(&CMD_LOCK); rwlock_init(&FS_LOCK)
    #define LOCK_READ rwlock_rdlock
    #define LOCK_WRITE rwlock_wrlock
    #define LOCK_UNLOCK rwlock_unlock
    #define DESTROY_LOCKS() rwlock_destroy(&CMD_LOCK); rwlock_destroy(&FS_LOCK)

    #define NOSYNC false
#else
    /*
       Nosync mode. These macros are only defined so that
       the compiler doesn't complain!
    */

    void* CMD_LOCK;
    void* FS_LOCK;
    #define INIT_LOCKS void_noarg
    #define LOCK_READ void_func
    #define LOCK_WRITE void_func
    #define LOCK_UNLOCK void_func
    #define DESTROY_LOCKS void_noarg

    #define NOSYNC true
#endif

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int headQueue = 0;
int numberCommands = 0;
int numberThreads = 0;
tecnicofs* fs;

static void parseArgs (int argc, char** const argv){
    // For the nosync edition, we are allowing the last argument
    // (num_threads) to be ommitted, since it's redundant

    if ((NOSYNC && argc != 4 && argc != 3) || (!NOSYNC && argc != 4)) {
        fprintf(stderr, red_bold("Invalid format!\n"));
        fprintf(stderr, red("Usage: %s %s %s %s\n"), argv[0], "input_file[.txt]", "output_file[.txt]", "num_threads");
        exit(EXIT_FAILURE);
    }

    if (NOSYNC) {
        // Display a warn if a thread argument was passed (and it is different than 1)
        if (argc == 4 && (argv[3][0] != '1' || argv[3][1] != '\0')) {
            fprintf(stderr, yellow_bold("This program is ran in no-sync mode (sequentially), which means that it only runs one thread.\n"));
        }
    } else {
        // Validates the number of threads, if in MT mode
        int threads = atoi(argv[3]);
        if (threads < 1) {
            fprintf(stderr, "%s\n%s %s\n", red_bold("Invalid number of threads!"), red("Expected a positive integer, got"), argv[3]);
            exit(EXIT_FAILURE);
        } else {
            fprintf(stderr, green("Spawning %d threads.\n\n"), threads);
            numberThreads = threads;
        }
    }
}

char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

void emitParseError(char* cmd){
    fprintf(stderr, "%s %s", yellow("Error! Invalid Command:"), cmd);
}

void processInput(FILE* input){
    char line[MAX_INPUT_SIZE];
    int errs = 0;

    while (fgets(line, sizeof(line)/sizeof(char), input)) {
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
                if(numTokens != 2){
                    emitParseError(line);
                    errs++;
                }
                if(insertCommand(line))
                    break;
            case '#':
                break;
            default: { /* error */
                emitParseError(line);
                errs++;
            }
        }
    }

    if (errs != 0) {
        fprintf(stderr, red_bold("\nParsing failure.\n"));
        exit(EXIT_FAILURE);
    }
}

void applyCommands(){
    char token;
    char name[MAX_INPUT_SIZE];

    while (numberCommands > 0) {
        LOCK_WRITE(&CMD_LOCK);
        const char* command = removeCommand();
        if (command == NULL){
            LOCK_UNLOCK(&CMD_LOCK);
            return;
        } else if (command[0] != 'c' && command[1] == ' ') {
            // Unlock early if the command doesn't require a new iNumber!
            LOCK_UNLOCK(&CMD_LOCK);
        }

        int numTokens = sscanf(command, "%c %s", &token, name);
        if (numTokens != 2) {
            fprintf(stderr, "%s %s\n", red_bold("Error! Invalid command in Queue:"), command);
            exit(EXIT_FAILURE);
        }

        int searchResult;
        int iNumber;
        switch (token) {
            case 'c':
                // We're now unlocking because we've got the iNumber!
                iNumber = obtainNewInumber(fs);
                LOCK_UNLOCK(&CMD_LOCK);

                LOCK_WRITE(&FS_LOCK);
                create(fs, name, iNumber);
                LOCK_UNLOCK(&FS_LOCK);

                break;
            case 'l':
                LOCK_READ(&FS_LOCK);
                searchResult = lookup(fs, name);

                if (!searchResult) {
                    printf("%s not found\n", name);
                } else {
                    printf("%s found with inumber %d\n", name, searchResult);
                }

                LOCK_UNLOCK(&FS_LOCK);
                break;
            case 'd':
                LOCK_WRITE(&FS_LOCK);
                delete(fs, name);
                LOCK_UNLOCK(&FS_LOCK);

                break;
            default: { /* error */
                fprintf(stderr, "%s %s\n", red_bold("Error! Invalid command in Queue:"), command);
                exit(EXIT_FAILURE);
            }
        }
    }
}

/*
    Redirects a thread to applyCommands() without having to
    deal with compiler warnings about wrong function types.
*/
void* applyCommandsLauncher(__attribute__ ((unused)) void* _) {
    applyCommands();

    pthread_exit(NULL);
    return NULL;
}

void deploy_threads() {
    if (NOSYNC) {
        // No need to apply any sort of commands, just run applyCommands-as-is
        applyCommands();
    } else {
        // Initialize the IO lock and local thread pool.
        INIT_LOCKS();
        pthread_t threadPool[numberThreads];

        for (int i = 0; i < numberThreads; i++) {
            if (pthread_create(&threadPool[i], NULL, applyCommandsLauncher, NULL)) {
                fprintf(stderr, red_bold("Failed to spawn the thread %d/%d!"), i, numberThreads);
                perror("\nError");
                exit(EXIT_FAILURE);
            }
        }

        // Wait until every thread is done.
        for (int i = 0; i < numberThreads; i++) {
            pthread_join(threadPool[i], NULL);
        }

        DESTROY_LOCKS();
    }
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
    processInput(cmds);
    fclose(cmds);

    // This time getting approach was found on https://stackoverflow.com/a/10192994
    fs = new_tecnicofs();

    gettimeofday(&start, NULL);
    deploy_threads();

    print_tecnicofs_tree(out, fs);
    fclose(out);

    free_tecnicofs(fs);
    gettimeofday(&end, NULL);

    double elapsed = (((double)(end.tv_usec - start.tv_usec)) / 1000000.0) + ((double)(end.tv_sec - start.tv_sec));

    fprintf(stderr, green_bold("TecnicoFS completed in %.04f seconds.\n"), elapsed);
    exit(EXIT_SUCCESS);
}
