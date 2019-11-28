/*

    File: socket.c
    Description: Implements functions that interact with sockets

*/

#include "err.h"
#include "socket.h"

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

socket_t newSocket(char* socketPath) {
    socket_t sock;
    sockaddr* server = malloc(sizeof(sockaddr));
    sockaddr* client = malloc(sizeof(sockaddr));
    int socketfd;
    errWrap(socketfd = socket(AF_UNIX, SOCK_STREAM, 0) < 0, "Unable to create socket!");
    errWrap(unlink(socketPath) < 0, "Unable to secure the socket name!");

    bzero((char*)server, sizeof(*server));
    bzero((char*)client, sizeof(*client));
    server -> sun_family = AF_UNIX;
    strcpy(server -> sun_path, socketPath);
    size_t servlen = strlen(server -> sun_path) + sizeof(server -> sun_family);

    errWrap(bind(socketfd, &server, servlen) < 0, "Unable to bind socket to local address!");
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

    errWrap(fork_fd = accept(sock.socket, sock.client, sizeof(sock.client)) < 0, "An error occurred while listening to incoming calls!");
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