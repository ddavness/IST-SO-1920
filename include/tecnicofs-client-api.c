/*

    File: tecnicofs-client-api.c
    Description: Client-sided TecnicoFS API

*/

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "tecnicofs-api-constants.h"
#include "tecnicofs-client-api.h"

typedef struct sockaddr_un sockaddr;

int currentSocketFD = 0;
int statuscode[1];

/*
    Internal function that sends a command to the tecnicofs server.
*/
int run(char* cmd, void* buff, size_t len) {
    if (!buff) {
        buff = statuscode;
        len = sizeof(int);
    }

    if (!currentSocketFD) {
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }

    if (send(currentSocketFD, cmd, (strlen(cmd) + 1) * sizeof(int), 0) < 0) {
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }

    if (read(currentSocketFD, buff, len) < 1) {
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }

    return *((int*)buff);
}

/*
    Mounts the client to the tecnicofs server via the socket provided by the address.
*/
int tfsMount(char* address) {
    if (currentSocketFD) {
        return TECNICOFS_ERROR_OPEN_SESSION;
    }

    sockaddr* server = malloc(sizeof(sockaddr));
    currentSocketFD = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(server, '\0', sizeof(*server));
    server -> sun_family = AF_UNIX;
    strcpy(server -> sun_path, address);

    if (connect(currentSocketFD, (struct sockaddr*)server, sizeof(*server))) {
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }

    return run("p 0 0", NULL, 0);
}

/*
    Unmounts the client from the current tecnicofs server.
*/
int tfsUnmount() {
    if (!currentSocketFD) {
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }

    if (close(currentSocketFD)) {
        return TECNICOFS_ERROR_OTHER;
    }

    currentSocketFD = 0;
    return TECNICOFS_OK;
}

/*
    Creates a file with the given name.

    Returns:
    - TECNICOFS_OK, if successful;
    - Error code, otherwise.
*/
int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions) {
    char* cmd = malloc((strlen(filename) + 6) * sizeof(char));
    sprintf(cmd, "c %s %d%d", filename, ownerPermissions, othersPermissions);
    int result = run(cmd, NULL, 0);
    free(cmd);
    return result;
}

/*
    Deletes the file with the given name, if it exists.

    Returns:
    - TECNICOFS_OK, if successful;
    - Error code, otherwise.
*/
int tfsDelete(char *filename) {
    char* cmd = malloc((strlen(filename) + 3) * sizeof(char));
    sprintf(cmd, "d %s", filename);
    int result = run(cmd, NULL, 0);
    free(cmd);
    return result;
}

/*
    Renames the given file to the new name.

    Returns:
    - TECNICOFS_OK, if successful;
    - Error code, otherwise.
*/
int tfsRename(char *filenameOld, char *filenameNew) {
    char* cmd = malloc((strlen(filenameOld) + strlen(filenameNew) + 4) * sizeof(char));
    sprintf(cmd, "r %s %s", filenameOld, filenameNew);
    int result = run(cmd, NULL, 0);
    free(cmd);
    return result;
}

/*
    Opens the given file in the given mode.

    Returns:
    - The file descriptor, if successful;
    - Error code, otherwise.
*/
int tfsOpen(char *filename, permission mode) {
    char* cmd = malloc((strlen(filename) + 5) * sizeof(char));
    sprintf(cmd, "o %s %d", filename, mode);
    int result = run(cmd, NULL, 0);
    free(cmd);
    return result;
}

/*
    Closes the given file descriptor.

    Returns:
    - TECNICOFS_OK, if successful;
    - Error code, otherwise.
*/
int tfsClose(int fd) {
    char* cmd = malloc(4 * sizeof(char));
    sprintf(cmd, "x %d", fd);
    int result = run(cmd, NULL, 0);
    free(cmd);
    return result;
}

/*
    Reads from a given file descriptor.

    Returns:
    - The number of characters actually read (0 <= x <= len), if successful;
    - Error code, otherwise.
*/
int tfsRead(int fd, char *buffer, int len) {
    void* out = malloc(sizeof(int) + len * sizeof(char));
    char* cmd = malloc(6 * sizeof(char));
    sprintf(cmd, "l %d %d", fd, len);
    int result = run(cmd, out, sizeof(int) + len * sizeof(char));
    strcpy(buffer, (char*)((intptr_t)out + (intptr_t)sizeof(int)));
    free(out);
    free(cmd);
    return result;
}

/*
    Writes to the file behind the given file descriptor.

    Returns:
    - TECNICOFS_OK, if successful;
    - Error code, otherwise.
*/
int tfsWrite(int fd, char *buffer, int len) {
    char* cmd = malloc((strlen(buffer) + 5) * sizeof(char));
    sprintf(cmd, "w %d %s", fd, buffer);
    int result = run(cmd, NULL, 0);
    free(cmd);
    return result;
}