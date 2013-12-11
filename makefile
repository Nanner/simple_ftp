CFLAGS = -Wall

DEPS = download.c ftpUtilities.c parseUtilities.c
OBJ  = download.o ftpUtilities.o parseUtilities.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

nserial: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIBS) -o $(BINDIR)download

.PHONY: clean dist all

clean:
	rm -f *.o *.a download