CCFLAGS=-Wall -O2

all: h5core

h5core: h5core.o
	$(CC) h5core.o -o $@

h5core.o: src/h5core.c
	$(CC) $(CCLAGS) -c $< -o $@

clean:
	rm -f *.o *.h5

veryclean: clean
	rm -f h5core *.chkexe *.chklog

TEST_NAME=h5core
TEST_CMD=sh ./scripts/h5core.sh -l 28

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

#	sh ./scripts/h5core.sh -l 28
	
