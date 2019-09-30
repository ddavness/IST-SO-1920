/*

    File: color.h
    Description: Describes functions for formatted (colored) output

*/

#include <stdbool.h>

#ifndef COLORTXT
#define COLORTXT

#define COLOR_MAX_ALLOC_OVERHEAD 16
#define COLOR_ESC '\x1b'
#define COLOR_TERM "[0m"
#define COLOR_STARTSEQ(BOLD) (BOLD ? "[1;" : "[")

/**
 * Appends the required extra bits in a string so that the terminal
 * displays it as formatted (colored) output. It isn't meant to be directly
 * used in the production code, please use the other functions provided in this library.
 *
 * @param string The string to be modified.
 * @param embolden Whether to embolden the string.
 * @param color_code The color code to apply to the string.
 * See https://misc.flogisoft.com/bash/tip_colors_and_formatting for details.
*/
char* addColorInfo(char* string, bool embolden, int color_code);

/**
 * Emboldens the given string while retaining the terminal's default color.
 * Don't use this on a string that is already colored, please see the color
 * functions.
 *
 * @param string The string to be emboldened.
*/
char* bold(char*);

/**
 * Turns the string into bright red.
 *
 * @param string The string to be colored.
 * @param bold Whether to also bolden the string.
*/
char* red(char* string, bool bold);

/**
 * Turns the string into bright green.
 *
 * @param string The string to be colored.
 * @param bold Whether to also bolden the string.
*/
char* green(char* string, bool bold);

/**
 * Turns the string into bright yellow.
 *
 * @param string The string to be colored.
 * @param bold Whether to also bolden the string.
*/
char* yellow(char* string, bool bold);

/**
 * Turns the string into bright blue (cyan).
 *
 * @param string The string to be colored.
 * @param bold Whether to also bolden the string.
*/
char* blue(char* string, bool bold);

#endif
