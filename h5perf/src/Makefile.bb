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
	rm -f h5perf *.chkexe *.chklog

TEST_NAME=h5perf
TEST_CMD=sh ./scripts/h5perf.sh

check:
	tname=$(TEST_NAME);\
        log=$${tname}.chklog; \
        echo "============================" | tee $${log}; \
        echo "Testing $(HDF5_DRIVER) $${tname} $(TEST_FLAGS)"; \
        echo "$(HDF5_DRIVER) $${tname} $(TEST_FLAGS) Test Log" | tee -a $${log}; \
        echo "============================" | tee -a $${log}; \
        srcdir="$(srcdir)" \
           $(TEST_CMD) | tee -a $${log} 2>&1 \
           && touch $${tname}.chkexe || \
           (test $$HDF5_Make_Ignore && echo "*** Error ignored") || \
           (cat $${log} && false) || exit 1; \
        echo "" | tee -a $${log}; \
        echo "Finished testing $${tname} $(TEST_FLAGS)" | tee -a $${log}; \
        echo "============================" | tee -a $${log}; \
        echo "Finished testing $${tname} $(TEST_FLAGS)";


#	sh ./scripts/h5perf.sh
	
