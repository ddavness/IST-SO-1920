/*

    File: tecnicofs-client-api.c
    Description: Client-sided TecnicoFS API

*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "tecnicofs-api-constants.h"
#include "tecnicofs-client-api.h"

#define MAX_PATH_LENGTH 108

typedef struct sockaddr_un sockaddr;

int currentSocketFD = 0;
char buff[] = "eeeeeeeeeeeez";
int statuscode[1];

/*
    Internal function that sends a command to the tecnicofs server.
*/
int sendCommand(char* cmd) {
    if (!currentSocketFD) {
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }

    if (send(currentSocketFD, cmd, (strlen(cmd) + 1)*sizeof(int), 0) < 0) {
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }

    if (read(currentSocketFD, statuscode, sizeof(int)) < 1) {
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }

    return *statuscode;
}

/*
    Mounts the client to the tecnicofs server via the socket provided by the address


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

    return TECNICOFS_OK;
}

/*
    Unmounts the client from the tecnicofs server.

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