CC=gcc
CFLAGS=-I. -lcrypto -lcurl
DEPS = bencode.h parser.h tracker.h
OBJ = main.o bencode.o parser.o tracker.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

torrent: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o *~ core *~