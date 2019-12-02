/*

    File: socket.h
    Description: Describes functions that interact with sockets

*/

#include <pthread.h>
#include <stdbool.h>
#include <sys/socket.h>

#ifndef TECNICOFS_SOCKET_H
#define TECNICOFS_SOCKET_H
#define MAX_PENDING_CALL_QUEUE 32

typedef int fdesc;

typedef struct sockaddr_un sockaddr;

typedef struct {
    fdesc socket;
    uid_t userId;
    pid_t procId;
    pthread_t* thread;
    sockaddr* server;
    sockaddr* client;
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
    bool* acceptCondition: a pointer to the condition that states whether to accept calls or not.

    Returns: the new thread that is handling the client.

    In case of error, the program automatically exits.
*/
socket_t acceptConnectionFrom(socket_t, bool*);

#endif
