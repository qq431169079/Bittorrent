CC=gcc
CFLAGS=-I. -lcrypto
DEPS = bencode.h parser.h
OBJ = main.o bencode.o parser.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

torrent: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o *~ core *~