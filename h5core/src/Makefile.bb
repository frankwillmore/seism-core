CCFLAGS=-Wall -O2

all: h5core

h5core: h5core.o
	$(CC) h5core.o -o $@

h5core.o: src/h5core.c
	$(CC) $(CCLAGS) -c $< -o $@

clean:
	rm -f *.o *.h5

veryclean: clean
	rm -f h5core

check:
	sh ./scripts/h5core.sh
	