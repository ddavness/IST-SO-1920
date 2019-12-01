/*

    File: locks.h
    Description: Describes primitives to handle errors
    in a standard and consistent way

*/

#ifndef ERR
#define ERR

#ifdef DEBUGMODE
    #define die abort()
#else
    #define die exit(EXIT_FAILURE)
#endif

void errWrap(int, char*);

#endif
