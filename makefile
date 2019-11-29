# Makefile, v1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20
#
# By: David Duque, 93698
# By: Filipe Ferro, 70611

CC = gcc
LD = gcc

SRC = src/
OUT = out/

CFLAGS = -g -pedantic -Wextra -Wall -Werror -std=gnu99 -I../
LDFLAGS= -lm -pthread

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean remake

all: tecnicofs-rwlock

# Final Program set

tecnicofs-mutex: out/bst.o out/err.o out/locks-mutex.o out/socket.o out/fs-mutex.o out/hash.o out/main-mutex.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-mutex out/bst.o out/err.o out/socket.o out/fs-mutex.o out/locks-mutex.o out/hash.o out/main-mutex.o

tecnicofs-rwlock: out/bst.o out/err.o out/locks-rwlock.o out/socket.o out/fs-rwlock.o out/hash.o out/main-rwlock.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-rwlock out/bst.o out/err.o out/socket.o out/fs-rwlock.o out/locks-rwlock.o out/hash.o out/main-rwlock.o

# Main variations (Nosync, Mutex, RWLock)

out/main-mutex.o: src/main.c src/fs.h src/lib/bst.h src/lib/color.h src/lib/locks.h src/lib/socket.h
	$(CC) $(CFLAGS) -DMUTEX -o out/main-mutex.o -c src/main.c

out/main-rwlock.o: src/main.c src/fs.h src/lib/bst.h src/lib/color.h src/lib/locks.h src/lib/socket.h
	$(CC) $(CFLAGS) -DRWLOCK -o out/main-rwlock.o -c src/main.c

# Sync variations

out/fs-mutex.o: src/fs.c src/fs.h src/lib/bst.h
	$(CC) $(CFLAGS) -DMUTEX -o out/fs-mutex.o -c src/fs.c

out/fs-rwlock.o: src/fs.c src/fs.h src/lib/bst.h
	$(CC) $(CFLAGS) -DRWLOCK -o out/fs-rwlock.o -c src/fs.c

# Lock variations

out/locks-mutex.o: src/lib/locks.c src/lib/locks.h
	$(CC) $(CFLAGS) -DMUTEX -o out/locks-mutex.o -c src/lib/locks.c

out/locks-rwlock.o: src/lib/locks.c src/lib/locks.h
	$(CC) $(CFLAGS) -DRWLOCK -o out/locks-rwlock.o -c src/lib/locks.c

# Dependencies

out/socket.o: src/lib/socket.c src/lib/socket.h
	$(CC) $(CFLAGS) -o out/socket.o -c src/lib/socket.c

out/hash.o: src/lib/hash.c src/lib/hash.h
	$(CC) $(CFLAGS) -o out/hash.o -c src/lib/hash.c

out/bst.o: src/lib/bst.c src/lib/bst.h
	$(CC) $(CFLAGS) -o out/bst.o -c src/lib/bst.c

out/err.o: src/lib/err.c src/lib/err.h
	$(CC) $(CFLAGS) -o out/err.o -c src/lib/err.c

# Misc

clean:
	@echo Cleaning...
	rm -f out/*.o out/*.o tecnicofs-*

remake:
	make clean
	make
