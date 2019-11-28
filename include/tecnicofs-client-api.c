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

/*
    Mounts the client to the tecnicofs server via the socket provided by the address


*/
int tfsMount(char* address) {
    sockaddr* server = malloc(sizeof(sockaddr));
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(server, '\0', sizeof(*server));
    server -> sun_family = AF_UNIX;
    strcpy(server -> sun_path, address);

    if (connect(sockfd, (struct sockaddr*)server, sizeof(*server)));
        perror("oof");
}

/*
    Unmounts the client from the tecnicofs server.

*/
int tfsUnmount() {

}