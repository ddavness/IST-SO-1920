/*

    File: socket.c
    Description: Implements functions that interact with sockets

*/

#define _GNU_SOURCE

#include "err.h"
#include "socket.h"

#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

socket_t newSocket(char* socketPath) {
    socket_t sock;
    sockaddr* server = malloc(sizeof(sockaddr));
    fdesc socketfd;
    errWrap((socketfd = socket(AF_UNIX, SOCK_STREAM, 0)) <= 0, "Unable to create socket!");
    int err = unlink(socketPath);
    errWrap(err < 0 && errno != ENOENT, "Unable to secure the socket name!");

    // Wipe all junk in the allocated memory
    memset(server, '\0', sizeof(*server));

    server -> sun_family = AF_UNIX;
    strcpy(server -> sun_path, socketPath);

    errWrap(bind(socketfd, (struct sockaddr *)server, sizeof(*server)) < 0, "Unable to bind socket to local address!");
    sock.socket = socketfd;
    sock.userId = -1;
    sock.procId = -1;
    sock.thread = NULL;
    sock.server = server;
    sock.client = NULL;

    errWrap(listen(socketfd, MAX_PENDING_CALL_QUEUE), "Unable to listen to clients!");
    return sock;
}

socket_t acceptConnectionFrom(socket_t sock, bool* acceptCondition) {
    socket_t fork;
    int fork_fd;
    socklen_t clientSize = (socklen_t)sizeof(sock.client);
    sockaddr* newclient = malloc(sizeof(sockaddr));
    memset(newclient, '\0', sizeof(*newclient));

    errWrap((fork_fd = accept(sock.socket, (struct sockaddr *)newclient, &clientSize)) < 0 && *acceptCondition, "An error occurred while listening to incoming calls!");
    if (!*acceptCondition) {
        free(newclient);
        fork.socket = -1;
        fork.thread = NULL;
        fork.client = NULL;
        fork.server = NULL;
        fork.procId = -1;
        fork.userId = -1;
        return fork;
    }
    fork.socket = fork_fd;
    fork.thread = malloc(sizeof(pthread_t));
    fork.client = newclient; // Fork will inherit the client information
    fork.server = NULL;

    struct ucred user;
    socklen_t usersize = sizeof(struct ucred);
    errWrap(getsockopt(fork_fd, SOL_SOCKET, SO_PEERCRED, &user, &usersize), "Failed to get user id!");
    fork.procId = user.pid;
    fork.userId = user.uid;

    return fork;
}
