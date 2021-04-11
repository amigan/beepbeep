# Beep-Beep Makefile

# select location to install
INSTALL_BIN = /usr/local/bin

BIN = ./beepbeep

# compiler flags
CC = gcc
LD = $(CC)
CFLAGS = -Wall -g
LIBS = -lm

all: $(BIN)
	@echo "All done"
	@exit

beepbeep.o: beepbeep.c Makefile
	$(CC) -c $(CFLAGS) beepbeep.c -o beepbeep.o

$(BIN):	beepbeep.o
	$(LD) $(LIBS) beepbeep.o -o $(BIN)

install:
	cp $(BIN) $(INSTALL_BIN)

clean:
	rm -f $(BIN)
	rm -f *.o
	rm -f .*.c.sw* .*.cpp.sw* .*.h.sw*
	rm -f bla nohup.out core

tar:
	make clean
	cd .. &&  tar -cvzf beepbeep.tar.gz beepbeep

