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

#include "lib/color.h"
#include "lib/void.h"
#include "fs.h"

// TODO: SWITCH TO THE CONDITIONAL COMPILATION METHOD
#define RWLOCK 1
#if defined(MUTEX)
    // Map macros to mutex

    pthread_mutex_t LOCK;
    #define LOCK_INIT pthread_mutex_init
    #define LOCK_READ pthread_mutex_lock
    #define LOCK_WRITE pthread_mutex_lock
    #define LOCK_OPEN pthread_mutex_unlock
    #define LOCK_DESTROY pthread_mutex_destroy

    #define NOSYNC false
#elif defined(RWLOCK)
    // Map macros to RWLOCK

    pthread_rwlock_t LOCK;
    #define LOCK_INIT pthread_rwlock_init
    #define LOCK_READ pthread_rwlock_rdlock
    #define LOCK_WRITE pthread_rwlock_wrlock
    #define LOCK_OPEN pthread_rwlock_unlock
    #define LOCK_DESTROY pthread_rwlock_destroy

    #define NOSYNC false
#else
    /*
       Nosync mode. These macros are only defined so that
       the compiler doesn't complain!
    */

    void* LOCK;
    #define LOCK_INIT void_2arg
    #define LOCK_READ void_1arg
    #define LOCK_WRITE void_1arg
    #define LOCK_OPEN void_1arg
    #define LOCK_DESTROY void_1arg

    #define NOSYNC true
#endif

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;
tecnicofs* fs;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

static void parseArgs (long argc, char** const argv){
    // For the nosync edition, we are allowing the last argument
    // (num_threads) to be ommitted, since it's redundant

    if ((NOSYNC && argc != 4 && argc != 3) || (!NOSYNC && argc != 4)) {
        fprintf(stderr, red_bold("Invalid format!\n"));
        fprintf(stderr, red("Usage: %s %s %s %s\n"), argv[0], "input_file[.txt]", "output_file[.txt]", "num_threads");
        exit(EXIT_FAILURE);
    }
}

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if (numberCommands > headQueue) {
        return inputCommands[headQueue++];  
    }
    return NULL;
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

void applyCommands(int begin, int hop){
    for (int i = begin; i < numberCommands; i += hop) {
        const char* command = removeCommand();
        if (command == NULL){
            break;
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
                LOCK_WRITE(&LOCK);
                iNumber = obtainNewInumber(fs);
                create(fs, name, iNumber);
                LOCK_OPEN(&LOCK);
                break;
            case 'l':
                LOCK_READ(&LOCK);
                searchResult = lookup(fs, name);
                
                if (!searchResult) {
                    printf("%s not found\n", name);
                } else {
                    printf("%s found with inumber %d\n", name, searchResult);
                }

                LOCK_OPEN(&LOCK);
                break;
            case 'd':
                LOCK_WRITE(&LOCK);
                delete(fs, name);
                LOCK_OPEN(&LOCK);
                break;
            default: { /* error */
                fprintf(stderr, "%s %c %s", red_bold("Error: Invalid command in Queue:\n"), token, name);
                exit(EXIT_FAILURE);
            }
        }
    }

    if (!NOSYNC) {
        pthread_exit(NULL);
    }
}

void* applyCommandsLauncher(void* argv) {
    int* arg = (int*)argv;
    applyCommands(arg[0], arg[1]);

    return NULL;
}

void deploy(char** argv) {
    if (NOSYNC) {
        // Display a warn if a thread argument was passed (and it is different than 1)
        if (argv[3] && (argv[3][0] != '1' || argv[3][1] != '\0')) {
            fprintf(stderr, yellow_bold("This program is ran in no-sync mode (sequentially), which means that it only runs one thread.\n"));
        }

        // No need to apply any sort of commands, just run applyCommands-as-is
        applyCommands(0, 1);
    } else {
        // Validates the number of threads
        int threads = atoi(argv[3]);
        if (threads < 1) {
            fprintf(stderr, "%s\n%s %s\n", red_bold("Invalid number of threads!"), red("Expected a positive integer, got"), argv[3]);
            exit(EXIT_FAILURE);
        } else {
            fprintf(stderr, green("Spawning %d threads.\n\n"), threads);
        }

        // Initialize the IO lock and local thread pool.
        LOCK_INIT(&LOCK, NULL);
        pthread_t threadPool[threads];

        for (int i = 0; i < threads; i++) {
            int* position = malloc(2 * sizeof(int));
            position[0] = i;
            position[1] = threads;
            pthread_create(threadPool + (pthread_t)i, NULL, applyCommandsLauncher, position);
        }

        // Wait until everyone is done.
        for (int i = 0; i < threads; i++) {
            pthread_join(threadPool[i], NULL);
        }

        LOCK_DESTROY(&LOCK);
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

    processInput(argv[1]);
    fs = new_tecnicofs();
    clock_t start = clock();

    deploy(argv);

    print_tecnicofs_tree(output, fs);
    fclose(output);

    free_tecnicofs(fs);
    clock_t end = clock();

    // TODO: CHECK WHETHER THIS IS A GOOD IDEA.
    double elapsed = ((double)(end - start)/CLOCKS_PER_SEC);

    printf(green_bold("\nTecnicoFS completed in %.04f seconds.\n"), elapsed);
    exit(EXIT_SUCCESS);
}
