CC=gcc
CFLAGS=-I.
DEPS = memory.h disassembler.h cpu8080.h motherboard.h debugger.h
OBJ = memory.o disassembler.o cpu8080.o motherboard.o debugger.o test_8080.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ core 
