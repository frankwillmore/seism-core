CCFLAGS=-DSTANDALONE -Wall -O2

all: h5perf

h5perf: pio_perf.o pio_engine.o pio_timer.o
	$(CC) -o $@ pio_perf.o pio_engine.o pio_timer.o 

pio_perf.o: src/pio_perf.c
	$(CC) $(CCFLAGS) -c $< -o $@

pio_engine.o: src/pio_engine.c
	$(CC) $(CCFLAGS) -c $< -o $@

pio_timer.o: src/pio_timer.c
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f *.o *.h5

veryclean: clean
	rm -f h5perf

check:
	sh ./scripts/h5perf.sh
	