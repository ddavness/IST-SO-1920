/* tecnicofs-api-constants.h */
#ifndef TECNICOFS_API_CONSTANTS_H
#define TECNICOFS_API_CONSTANTS_H

typedef enum permission { NONE, WRITE, READ, RW } permission;

#define MAX_OPEN_FILES 5

/* Operation successful */
#define TECNICOFS_OK 0

/* Client already has an open session with a TecnicoFS server */
#define TECNICOFS_ERROR_OPEN_SESSION -1
/* Doesn't exist an open session */
#define TECNICOFS_ERROR_NO_OPEN_SESSION -2
/* Communication failed */
#define TECNICOFS_ERROR_CONNECTION_ERROR -3
/* Already exists a file with the given name */
#define TECNICOFS_ERROR_FILE_ALREADY_EXISTS -4
/* No file found with the given name */
#define TECNICOFS_ERROR_FILE_NOT_FOUND -5
/* Client doesn't have permissions for the operation */
#define TECNICOFS_ERROR_PERMISSION_DENIED -6
/* Number of open files that can be open has been reached */
#define TECNICOFS_ERROR_MAXED_OPEN_FILES -7
/* File is not open */
#define TECNICOFS_ERROR_FILE_NOT_OPEN -8
/* File is open */
#define TECNICOFS_ERROR_FILE_IS_OPEN -9
/* File is open in the a mode that doesn't allow the operation */
#define TECNICOFS_ERROR_INVALID_MODE -10
/* Command has been emitted with bad syntax */
#define TECNICOFS_ERROR_INVALID_SYNTAX -11
/* Generic error */
#define TECNICOFS_ERROR_OTHER -12

#endif /* TECNICOFS_API_CONSTANTS_H */
