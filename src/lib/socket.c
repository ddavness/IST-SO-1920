/*

    File: socket.c
    Description: Implements functions that interact with sockets

*/

#include "err.h"
#include "socket.h"

#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

socket_t newSocket(char* socketPath) {
    socket_t sock;
    sockaddr server;
    int socketfd;
    errWrap(socketfd = socket(AF_UNIX, SOCK_STREAM, 0) < 0, "Unable to create socket!");
    errWrap(unlink(socketPath) < 0, "Unable to secure the socket name!");

    bzero((char*)&server, sizeof(server));
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, socketPath);
    size_t servlen = strlen(server.sun_path) + sizeof(server.sun_family);

    errWrap(bind(socketfd, &server, servlen) < 0, "Unable to bind socket to local address!");
    sock.socket = socketfd;
    sock.server = server;

    errWrap(listen(socketfd, MAX_PENDING_CALL_QUEUE), "Unable to listen to clients!");
    return sock;
}

pthread_t* acceptConnectionFrom(socket_t sock) {
    
}