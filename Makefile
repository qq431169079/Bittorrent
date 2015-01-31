CC=gcc
CFLAGS=-I. -lcrypto -lcurl -lpthread
DEPS = bencode.h parser.h tracker.h parser.h
OBJ = main.o bencode.o parser.o tracker.o decode.o encode.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

torrent: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o *~ core *~