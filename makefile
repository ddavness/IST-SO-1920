# Makefile, v1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20
#
# By: David Duque, 93698
# By: Filipe Ferro, 70611

CC = gcc
LD = gcc

SRC = src/
OUT = out/

CFLAGS = -g -pedantic -Wextra -Wall -std=gnu99 -I../
LDFLAGS= -lm -pthread

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean remake pkg

all: tecnicofs-rwlock
	mv tecnicofs-rwlock tecnicofs

# Final Program set

tecnicofs-mutex: out/memutils.o out/bst.o out/err.o out/locks-mutex.o out/socket.o out/fs-mutex.o out/hash.o out/inodes.o out/cmd-mutex.o out/main-mutex.o
	$(LD) $(LDFLAGS) -o tecnicofs-mutex out/memutils.o out/bst.o out/err.o out/socket.o out/fs-mutex.o out/locks-mutex.o out/hash.o out/inodes.o out/cmd-mutex.o out/main-mutex.o

tecnicofs-rwlock: out/memutils.o out/bst.o out/err.o out/locks-rwlock.o out/socket.o out/fs-rwlock.o out/hash.o out/inodes.o out/cmd-rwlock.o out/main-rwlock.o
	$(LD) $(LDFLAGS) -o tecnicofs-rwlock out/memutils.o out/bst.o out/err.o out/socket.o out/fs-rwlock.o out/locks-rwlock.o out/hash.o out/inodes.o out/cmd-rwlock.o out/main-rwlock.o

# Main variations (Mutex, RWLock)

out/main-mutex.o: src/main.c src/fs.h src/lib/bst.h src/lib/color.h src/lib/locks.h src/lib/socket.h
	$(CC) $(CFLAGS) -DMUTEX -o out/main-mutex.o -c src/main.c

out/main-rwlock.o: src/main.c src/fs.h src/lib/bst.h src/lib/color.h src/lib/locks.h src/lib/socket.h
	$(CC) $(CFLAGS) -DRWLOCK -o out/main-rwlock.o -c src/main.c

# applyCommands() variations

out/cmd-mutex.o: src/cmd.c src/lib/err.c src/lib/inodes.c src/lib/socket.c
	$(CC) $(CFLAGS) -DMUTEX -o out/cmd-mutex.o -c src/cmd.c

out/cmd-rwlock.o: src/cmd.c src/lib/err.c src/lib/inodes.c src/lib/socket.c
	$(CC) $(CFLAGS) -DRWLOCK -o out/cmd-rwlock.o -c src/cmd.c

# FS variations

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

out/memutils.o: src/lib/memutils.c src/lib/memutils.h
	$(CC) $(CFLAGS) -o out/memutils.o -c src/lib/memutils.c

out/socket.o: src/lib/socket.c src/lib/socket.h
	$(CC) $(CFLAGS) -o out/socket.o -c src/lib/socket.c

out/hash.o: src/lib/hash.c src/lib/hash.h
	$(CC) $(CFLAGS) -o out/hash.o -c src/lib/hash.c

out/bst.o: src/lib/bst.c src/lib/bst.h
	$(CC) $(CFLAGS) -o out/bst.o -c src/lib/bst.c

out/err.o: src/lib/err.c src/lib/err.h
	$(CC) $(CFLAGS) -o out/err.o -c src/lib/err.c

out/inodes.o: src/lib/inodes.c src/lib/inodes.h
	$(CC) $(CFLAGS) -o out/inodes.o -c src/lib/inodes.c

# Misc

clean:
	rm -f out/*.o out/*.o tecnicofs-* tecnicofs
	rm -rf client
	rm -rf server

remake:
	make clean
	make

pkg:
	make clean
	mkdir -p server
	mkdir -p server/out
	cp -rf src/ server/
	cp makefile server/
	mkdir -p client
	cp include/*.c include/*.h client/
	zip -9 proj-so-ex3-g84.zip -r server/ client/
