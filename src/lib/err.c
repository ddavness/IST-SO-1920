/*

    File: locks.h
    Description: Implements primitives to handle errors
    in a standard and consistent way

*/

#include "color.h"
#include "err.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void errWrap(int errCondition, char* msg) {
    if (errCondition) {
        fprintf(stderr, red_bold("%s"), msg);
        perror("\nError");
        exit(EXIT_FAILURE);
    }
}