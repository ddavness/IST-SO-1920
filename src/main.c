#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>
#include <pthread.h>
#include <sys/types.h>

#include "lib/color.h"
#include "fs.h"

#define RWLOCK 1
#if defined(MUTEX)
    // Map macros to mutex

    pthread_mutex_t* LOCK;
    #define LOCK_INIT pthread_mutex_init
    #define LOCK_CLOSE_READ pthread_mutex_lock
    #define LOCK_CLOSE_WRITE pthread_mutex_lock
    #define LOCK_OPEN pthread_mutex_unlock
    #define LOCK_DESTROY pthread_mutex_destroy

    #define NOSYNC false
#elif defined(RWLOCK)
    // Map macros to RWLOCK

    pthread_rwlock_t* LOCK;
    #define LOCK_INIT pthread_rwlock_init
    #define LOCK_CLOSE_READ pthread_rwlock_rdlock
    #define LOCK_CLOSE_WRITE pthread_rwlock_wrlock
    #define LOCK_OPEN pthread_rwlock_unlock
    #define LOCK_DESTROY pthread_rwlock_destroy

    #define NOSYNC false
#else
    /*
       Nosync. These macros are only defined so that the compiler
       doesn't complain that much.
    */

    void* LOCK;
    #define LOCK_INIT NULL
    #define LOCK_CLOSE_READ NULL
    #define LOCK_CLOSE_WRITE NULL
    #define LOCK_OPEN NULL
    #define LOCK_DESTROY NULL

    #define NOSYNC true
#endif

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

struct ERR_MSG {
    char* USAGE_ARGS_FMT;   // Show the user how to use the program
    char* BAD_ARGS;         // The program was invoked with the wrong arguments - How mean!
    char* BAD_CMD;          // The input file contains a bad command
    char* IO_ERR;           // Problems with opening files (e.g. ENOENT, Access Denied, etc.)
}ERR_MSG;

int numberThreads = 0;
tecnicofs* fs;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

static void parseArgs (long argc, char** const argv){
    if (argc != 4) {
        fprintf(stderr, ERR_MSG.BAD_ARGS);
        fprintf(stderr, ERR_MSG.USAGE_ARGS_FMT, argv[0], "input_file[.txt]", "output_file[.txt]", "num_threads");
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
    if (numberCommands > 0) {
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

void errorParse(char token, char* name){
    fprintf(stderr, "%s %c %s\n", ERR_MSG.BAD_CMD, token, name);
    //exit(EXIT_FAILURE);
}

void processInput(char* input){
    char line[MAX_INPUT_SIZE];
    FILE* file = fopen(input, "r");
    int errs = 0;

    if (!file) {
        fprintf(stderr, ERR_MSG.IO_ERR, input);
        perror("\n");
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
        fprintf(stderr, red("Exiting\n", true));
        exit(EXIT_FAILURE);
    }

    // Do not forget to close the file!
    fclose(file);
}

void applyCommands(begin, hop){
    while(numberCommands > 0){
        const char* command = removeCommand();
        if (command == NULL){
            continue;
        }

        char token;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s", &token, name);
        if (numTokens != 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        int iNumber;
        switch (token) {
            case 'c':
                iNumber = obtainNewInumber(fs);
                create(fs, name, iNumber);
                break;
            case 'l':
                searchResult = lookup(fs, name);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                break;
            case 'd':
                delete(fs, name);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

int main(int argc, char** argv) {
    // Deploy the error messages
    ERR_MSG.USAGE_ARGS_FMT = red("Usage: %s %s %s %s\n", false);
    ERR_MSG.BAD_ARGS = red("Invalid format!\n", true);
    ERR_MSG.BAD_CMD = yellow("Error: Invalid Command!", false);
    ERR_MSG.IO_ERR = red("Unable to open file '%s'!", true);

    parseArgs(argc, argv);

    processInput(argv[1]);
    fs = new_tecnicofs();
    clock_t start = clock();

    if (NOSYNC) {
        // No need to apply any sort of commands, just run applyCommands-as-is
        applyCommands();
    } else {
        // Initialize the IO lock.
        LOCK_INIT(LOCK, NULL);
        applyCommands();
    }

    print_tecnicofs_tree(stdout, fs);

    free_tecnicofs(fs);
    clock_t end = clock();

    // I'm not entirely sure whether this is a good approach!
    // TODO: CHECK WHETHER THIS IS A GOOD IDEA.
    double elapsed = ((double)(end - start)/CLOCKS_PER_SEC);

    printf("\nTecnicoFS completed in %.04f seconds.\n", elapsed);

    free(ERR_MSG.USAGE_ARGS_FMT);
    free(ERR_MSG.BAD_ARGS);
    free(ERR_MSG.BAD_CMD);
    free(ERR_MSG.IO_ERR);
    exit(EXIT_SUCCESS);
}
