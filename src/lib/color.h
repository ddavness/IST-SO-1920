/*

    File: color.h
    Description: Describes macros for formatted (colored) output

*/

#include <stdbool.h>

#ifndef COLORTXT
#define COLORTXT

#define COLOR_RESET "\x1b[0m"
#define COLOR_RED "\x1b[91m"
#define COLOR_YELLOW "\x1b[93m"
#define COLOR_RED_BOLD "\x1b[1;91m"
#define COLOR_YELLOW_BOLD "\x1b[1;93m"


#define red(str) (COLOR_RED str COLOR_RESET)
#define yellow(str) (COLOR_YELLOW str COLOR_RESET)
#define red_bold(str) (COLOR_RED_BOLD str COLOR_RESET)
#define yellow_bold(str) (COLOR_YELLOW_BOLD str COLOR_RESET)

#endif
