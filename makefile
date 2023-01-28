CC=gcc
CFLAGS=-I.
DEPS = memory.h disassembler.h
OBJ = memory.o disassembler.o test_8080.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ core 
