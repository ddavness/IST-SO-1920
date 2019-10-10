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

static void parseArgs (long argc, char** const argv){
    // For the nosync edition, we are allowing the last argument
    // (num_threads) to be ommitted, since it's redundant

    if ((NOSYNC && argc != 4 && argc != 3) || (!NOSYNC && argc != 4)) {
        fprintf(stderr, red_bold("Invalid format!\n"));
        fprintf(stderr, red("Usage: %s %s %s %s\n"), argv[0], "input_file[.txt]", "output_file[.txt]", "num_threads");
        exit(EXIT_FAILURE);
    }

    if (NOSYNC) {
        // Display a warn if a thread argument was passed (and it is different than 1)
        if (argv[3] && (argv[3][0] != '1' || argv[3][1] != '\0')) {
            fprintf(stderr, yellow_bold("This program is ran in no-sync mode (sequentially), which means that it only runs one thread.\n"));
        }
    } else {
        // Validates the number of threads
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

void errorParse(char token, char* name){
    fprintf(stderr, "%s %c %s\n", yellow("Error: Invalid Command!"), token, name);
}

void processInput(char* input){
    char line[MAX_INPUT_SIZE];
    FILE* file = fopen(input, "r");
    int errs = 0;

    if (!file) {
        fprintf(stderr, red_bold("Unable to open file '%s'!"), input);
        perror("\nError");
        exit(EXIT_FAILURE);
    }

    while (fgets(line, sizeof(line)/sizeof(char), file)) {
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
                    errorParse(token, name);
                    errs++;
                }
                if(insertCommand(line))
                    break;
            case '#':
                break;
            default: { /* error */
                errorParse(token, name);
                errs++;
            }
        }
    }

    if (errs != 0) {
        fprintf(stderr, red_bold("Exiting\n"));
        exit(EXIT_FAILURE);
    }

    // Do not forget to close the file!
    fclose(file);
}

void applyCommands(){
    while (numberCommands > 0) {
        LOCK_WRITE(&CMD_LOCK);
        const char* command = removeCommand();
        if (command == NULL){
            break;
        } else if ((command[0] == 'l' || command[0] == 'd') && command[1] == ' ') {
            // Unlock early if the command doesn't require a new iNumber!
            LOCK_UNLOCK(&CMD_LOCK);
        }

        char token;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s", &token, name);
        if (numTokens != 2) {
            fprintf(stderr, "%s %c %s", red_bold("Error: Invalid command in Queue:\n"), token, name);
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
                fprintf(stderr, "%s %c %s", red_bold("Error: Invalid command in Queue:\n"), token, name);
                exit(EXIT_FAILURE);
            }
        }
    }
}

void* applyCommandsLauncher(__attribute__ ((unused)) void* argv) {
    applyCommands();

    pthread_exit(NULL);
    return NULL;
}

void deploy(char** argv) {
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

    // Try to open the output file, so that we can catch the error early!
    // Possible errors when opening/creating the file: Access denied
    FILE* output = fopen(argv[2], "w");
    if (!output) {
        fprintf(stderr, red_bold("Unable to open file '%s'!"), argv[2]);
        perror("\nError");
        exit(EXIT_FAILURE);
    }

    struct timespec start, end;
    processInput(argv[1]);

    // This time getting approach was found on https://stackoverflow.com/a/10192994
    clock_gettime(CLOCK_MONOTONIC, &start);
    fs = new_tecnicofs();

    deploy(argv);

    print_tecnicofs_tree(output, fs);
    fclose(output);

    free_tecnicofs(fs);
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (((double)(end.tv_nsec - start.tv_nsec)) / 1000000000.0) + ((double)(end.tv_sec - start.tv_sec));

    fprintf(stderr, green_bold("\nTecnicoFS completed in %.04f seconds.\n"), elapsed);
    exit(EXIT_SUCCESS);
}
