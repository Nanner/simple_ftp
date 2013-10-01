CFLAGS = -Wall
BINDIR = ./

DEPS= supervisionFrame.h
OBJ_SHARED = supervisionFrame.o
OBJ_SENDER = writenoncanonical.o
OBJ_RECEIVER = noncanonical.o

all: sender receiver

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sender: $(OBJ_SENDER) $(OBJ_SHARED)
	$(CC) $(CFLAGS) $(OBJ_SENDER) $(OBJ_SHARED) -o $(BINDIR)/sender

receiver: $(OBJ_RECEIVER) $(OBJ_SHARED)
	$(CC) $(CFLAGS) $(OBJ_RECEIVER) $(OBJ_SHARED) -o $(BINDIR)/receiver

.PHONY: clean dist all

clean:
	rm -f *.o *.a receiver sender

dist: 
	cd .. && tar -zcvf T1G05.tar.gz T1G05