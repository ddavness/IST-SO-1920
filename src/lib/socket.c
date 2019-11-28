/*

    File: socket.c
    Description: Implements functions that interact with sockets

*/

#include "err.h"
#include "socket.h"

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

socket_t newSocket(char* socketPath) {
    socket_t sock;
    sockaddr* server = malloc(sizeof(sockaddr));
    sockaddr* client = malloc(sizeof(sockaddr));
    fdesc socketfd;
    errWrap(socketfd = socket(AF_UNIX, SOCK_STREAM, 0) < 0, "Unable to create socket!");
    int err = unlink(socketPath);
    errWrap(err < 0 && errno != ENOENT, "Unable to secure the socket name!");

    // Wipe all junk in the allocated memory
    bzero((char*)server, sizeof(*server));
    bzero((char*)client, sizeof(*client));

    server -> sun_family = AF_UNIX;
    strcpy(server -> sun_path, socketPath);

    errWrap(bind(socketfd, (struct sockaddr *)server, sizeof(*server)) < 0, "Unable to bind socket to local address!");
    sock.socket = socketfd;
    sock.thread = NULL;
    sock.server = server;
    sock.client = client;

    errWrap(listen(socketfd, MAX_PENDING_CALL_QUEUE), "Unable to listen to clients!");
    return sock;
}

socket_t acceptConnectionFrom(socket_t sock) {
    socket_t fork;
    int fork_fd;
    unsigned int client_size = (unsigned int)sizeof(sock.client);

    errWrap(fork_fd = accept(sock.socket, (struct sockaddr *)sock.client, &client_size) < 0, "An error occurred while listening to incoming calls!");
    fork.socket = fork_fd;
    fork.thread = malloc(sizeof(pthread_t));
    fork.client = sock.client; // Fork will inherit the client information
    fork.server = NULL;

    // Create another clean client address for the main socket
    sockaddr* newclient = malloc(sizeof(sockaddr));
    bzero((char*)newclient, sizeof(*newclient));
    sock.client = newclient;

    return fork;
}