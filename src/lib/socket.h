/*

    File: socket.h
    Description: Describes functions that interact with sockets

*/

#include <sys/socket.h>
#include <pthread.h>

#ifndef TECNICOFS_SOCKET_H
#define TECNICOFS_SOCKET_H
#define MAX_PENDING_CALL_QUEUE 32

typedef struct sockaddr_un sockaddr;

typedef struct {
    int socket;
    sockaddr server;
} socket_t;

/*
    Creates a new socket that is listening.

    char* socketPath: the path where to store the socket. It is suggested to be chosen in the /tmp directory.

    Returns: a new socket_t struct

    In case of error, the program automatically exits.
*/
socket_t newSocket(char*);

/*
    Accepts and handles a new process connecting to the socket.

    socket_t socket: the socket you're accepting the call from.

    Returns: the new thread that is handling the client.

    In case of error, the program automatically exits.
*/
pthread_t* acceptConnectionFrom(socket_t);

#endif