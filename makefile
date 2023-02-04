CC=gcc
CFLAGS=-I/usr/include/SDL2 -I. 
LINKER_FLAGS = -lSDL2 -lSDL2_mixer
DEPS = memory.h disassembler.h cpu8080.h motherboard.h debugger.h
TEST_OBJ = memory.o disassembler.o cpu8080.o motherboard.o debugger.o test_8080.o
SPACE_OBJ = memory.o disassembler.o cpu8080.o motherboard.o debugger.o space_invaders.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

space: $(SPACE_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LINKER_FLAGS)

test: $(TEST_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ core 
