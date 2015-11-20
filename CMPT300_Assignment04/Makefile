## Basic Info
##------------------------------------------------
##           my name : Jingda(Kingston) Chen    
##    student number : 301295759                
##     SFU user name : jca303                   
##   lecture section : CMPT 300 D100, Fall 2015 
## instructor's name : Brian Booth              
##         TA's name : Scott Kristjanson        

CC=gcc
CFLAGS=-I.

DEPS = gettime.h memwatch.h
OBJ = memwatch.o gettime.o lyrenest.o

%.o: %.c $(DEPS)
	$(CC) -DMEMWATCH -DMW_STDIO -c -g -o $@ $< $(CFLAGS)

lyrenest: $(OBJ)
	gcc -DMEMWATCH -DMW_STDIO -g -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJ)
	rm -f core
	rm -f lyrenest
