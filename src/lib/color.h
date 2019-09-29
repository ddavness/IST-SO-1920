/*

    File: color.h
    Description: Describes functions for formatted (colored) output

*/

#include "default.h"

#ifndef COLORTXT
#define COLORTXT

#define COLOR_MAX_ALLOC_OVERHEAD 16
#define COLOR_ESC '\x1b'
#define COLOR_TERM "[0m"
#define COLOR_STARTSEQ(BOLD) (BOLD ? "[1;" : "[")

char* addColorInfo(char*, bool, int);

char* bold(char*);

char* red(char*, bool);
char* green(char*, bool);
char* yellow(char*, bool);
char* blue(char*, bool);

#endif
