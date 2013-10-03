CFLAGS = -Wall
BINDIR = ./

DEPS = main.h supervisionFrame.h linkLayerProtocol.h
OBJ  = main.o supervisionFrame.o linkLayerProtocol.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

nserial: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIBS) -o $(BINDIR)nserial

.PHONY: clean dist all

clean:
	rm -f *.o *.a nserial

dist: 
	cd .. && tar -zcvf T1G05.tar.gz T1G05