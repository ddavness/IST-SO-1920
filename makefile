# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

CC = gcc
LD = gcc

SRC = src/
OUT = out/

CFLAGS = -pedantic -Wall -Wextra -Werror -std=gnu99 -g -I../
LDFLAGS= -lm -lpthread

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run remake nosync

all: tecnicofs

tecnicofs: out/lib/bst.o out/lib/color.o out/fs.o out/main.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs out/lib/bst.o out/lib/color.o out/fs.o out/main.o

out/lib/bst.o: src/lib/bst.c src/lib/bst.h
	$(CC) $(CFLAGS) -o out/lib/bst.o -c src/lib/bst.c

out/lib/color.o: src/lib/color.c src/lib/color.h
	$(CC) $(CFLAGS) -o out/lib/color.o -c src/lib/color.c

out/fs.o: src/fs.c src/fs.h src/lib/bst.h
	$(CC) $(CFLAGS) -o out/fs.o -c src/fs.c

out/main.o: src/main.c src/fs.h src/lib/bst.h src/lib/color.h
	$(CC) $(CFLAGS) -o out/main.o -c src/main.c

clean:
	@echo Cleaning...
	rm -f out/lib/*.o out/*.o tecnicofs

remake:
	make clean
	make

run: tecnicofs
	./tecnicofs
