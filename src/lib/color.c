/*

    File: color.h
    Description: Implements functions for formatted (colored) output

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "color.h"

char* addColorInfo(char* text, bool makeBold, int color) {
    char* out = malloc((strlen(text) + COLOR_MAX_ALLOC_OVERHEAD) * sizeof(char));
    sprintf(out, "%c%s%dm%s%c%s", COLOR_ESC, COLOR_STARTSEQ(makeBold), color, text, COLOR_ESC, COLOR_TERM);

    return out;
}

char* bold(char* txt) { return addColorInfo(txt, true, 39); }

char* red(char* txt, bool bold) { return addColorInfo(txt, bold, 91); }
char* green(char* txt, bool bold) { return addColorInfo(txt, bold, 92); }
char* yellow(char* txt, bool bold) { return addColorInfo(txt, bold, 93); }
char* blue(char* txt, bool bold) { return addColorInfo(txt, bold, 94); }
