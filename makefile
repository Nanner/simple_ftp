CFLAGS = -Wall
BINDIR = ./

DEPS = main.h frame.h applicationLayer.h linkLayerProtocol.h
OBJ  = main.o frame.o applicationLayer.o linkLayerProtocol.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

nserial: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIBS) -o $(BINDIR)nserial

.PHONY: clean dist all

clean:
	rm -f *.o *.a nserial

dist: 
	cd .. && tar -zcvf T1G05.tar.gz T1G05