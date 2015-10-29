IDIR =../include
ODIR =../build
BDIR =../bin

CC=gcc
CFLAGS=-I$(IDIR)

_DEPS = readline.h decrypt.h memwatch.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = memwatch.o readline.o decrypt.o main.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -DMEMWATCH -DMW_STDIO -c -o $@ $< $(CFLAGS)

$(BDIR)/lyrebird: $(OBJ)
	gcc -DMEMWATCH -DMW_STDIO -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJ)
	rm -f core
