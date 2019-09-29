# This is the makefile for the TecnicoFS project.
#
# By: David Duque, nº 93698
#     Filipe Ferro, nº 70611

CC = gcc
CFLAGS = -std=c11 -pedantic -Wall -Wextra -Werror
CFLAGS_DEBUG = -g
CFLAGS_OPTIMIZE = -O3

# We're compiling the main source and utilities that we wrote.
FILES = src/*.c src/lib/*.c

make:
	$(CC) $(FILES) $(CFLAGS) -o tecnicofs

debug:
	$(CC) $(FILES) $(CFLAGS) $(CFLAGS_DEBUG) -o tecnicofs