# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

CC = gcc
LD = gcc

SRC = src/
OUT = out/

CFLAGS = -pedantic -Wall -Wextra -Werror -std=gnu99 -g -I../
LDFLAGS= -lm -pthread

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run remake nosync

all: tecnicofs-nosync tecnicofs-mutex tecnicofs-rwlock

# Final Program

tecnicofs-nosync: out/lib/bst.o out/lib/void.o out/fs.o out/main-nosync.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-nosync out/lib/bst.o out/lib/void.o out/fs.o out/main-nosync.o

tecnicofs-mutex: out/lib/bst.o out/lib/void.o out/fs.o out/main-mutex.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-mutex out/lib/bst.o out/lib/void.o out/fs.o out/main-mutex.o

tecnicofs-rwlock: out/lib/bst.o out/lib/void.o out/fs.o out/main-rwlock.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-rwlock out/lib/bst.o out/lib/void.o out/fs.o out/main-rwlock.o

# Main variations (Nosync, Mutex, RWLock)

out/main-nosync.o: src/main.c src/fs.h src/lib/bst.h src/lib/color.h src/lib/void.h
	$(CC) $(CFLAGS) -o out/main-nosync.o -c src/main.c

out/main-mutex.o: src/main.c src/fs.h src/lib/bst.h src/lib/color.h src/lib/void.h
	$(CC) $(CFLAGS) -DMUTEX -o out/main-mutex.o -c src/main.c

out/main-rwlock.o: src/main.c src/fs.h src/lib/bst.h src/lib/color.h src/lib/void.h
	$(CC) $(CFLAGS) -DRWLOCK -o out/main-rwlock.o -c src/main.c

# Dependencies

out/lib/bst.o: src/lib/bst.c src/lib/bst.h
	$(CC) $(CFLAGS) -o out/lib/bst.o -c src/lib/bst.c

out/fs.o: src/fs.c src/fs.h src/lib/bst.h
	$(CC) $(CFLAGS) -o out/fs.o -c src/fs.c

out/lib/void.o: src/lib/void.c src/lib/void.h
	$(CC) $(CFLAGS) -o out/lib/void.o -c src/lib/void.c

# Misc

clean:
	@echo Cleaning...
	rm -f out/lib/*.o out/*.o tecnicofs-*

remake:
	make clean
	make
