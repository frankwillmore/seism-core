CC=h5pcc
#CCFLAGS=-DSTANDALONE -Wall -O2
# craycc returns: ERROR in command line
# The phase argument or the comma before the phase argument is 
# missing for the -W option.
CCFLAGS=-DSTANDALONE -Wall -O2

all: h5perf

h5perf: pio_perf.o pio_engine.o pio_timer.o
	$(CC) -o $@ pio_perf.o pio_engine.o pio_timer.o 

pio_perf.o: pio_perf.c
	$(CC) $(CCFLAGS) -c $< -o $@

pio_engine.o: pio_engine.c
	$(CC) $(CCFLAGS) -c $< -o $@

pio_timer.o: pio_timer.c
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f *.o *.h5

veryclean: clean
	rm -f h5perf
