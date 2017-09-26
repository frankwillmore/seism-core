
# Should not need to modify anything below...

FFLAGS=-O2

all: kernel

kernel: KERNEL.o
	$(FC) IO.o KERNEL.o -o kernel.x

KERNEL.o: IO.o src/KERNEL.F90
	$(FC) $(FFLAGS) -c src/KERNEL.F90

IO.o: src/IO.F90
	$(FC) $(FFLAGS) -c src/IO.F90

clean:
	rm -f *.o *.mod

veryclean: clean
	rm -f *.x *~ *.chkexe *.chklog

TEST_NAME=combust_io
TEST_CMD=sh ./scripts/kernel.sh

check:
	tname=$(TEST_NAME);\
        log=$${tname}.chklog; \
        echo "============================" > $${log}; \
        echo "Testing $(HDF5_DRIVER) $${tname} $(TEST_FLAGS)"; \
        echo "$(HDF5_DRIVER) $${tname} $(TEST_FLAGS) Test Log" >> $${log}; \
        echo "============================" >> $${log}; \
        srcdir="$(srcdir)" \
           $(TEST_CMD) >> $${log} 2>&1 \
           && touch $${tname}.chkexe || \
           (test $$HDF5_Make_Ignore && echo "*** Error ignored") || \
           (cat $${log} && false) || exit 1; \
        echo "" >> $${log}; \
        echo "Finished testing $${tname} $(TEST_FLAGS)" >> $${log}; \
        echo "============================" >> $${log}; \
        echo "Finished testing $${tname} $(TEST_FLAGS)"; \
        cat $${log}; 

#	sh ./scripts/kernel.sh
#                 $(TIME) $(RUNEXEC) ./$${tname} $(TEST_FLAGS) >> $${log} 2>&1 \
