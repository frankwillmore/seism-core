
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

TEST_NAME=kernel.x
TEST_CMD=$(RUNEXEC) $(TEST_NAME)

check:
	tname=$(TEST_NAME);\
        log=$${tname}.chklog; \
        echo "============================" | tee $${log}; \
        echo "Testing $(HDF5_DRIVER) $${tname} $(TEST_FLAGS)"; \
        echo "$(HDF5_DRIVER) $${tname} $(TEST_FLAGS) Test Log" | tee -a $${log}; \
        echo "============================" | tee -a $${log}; \
	sh ./prep_data.bb 4
	srcdir="$(srcdir)" \
           $(RUNPARALLEL) $(TEST_CMD) | tee -a $${log} 2>&1 \
           && touch $${tname}.chkexe || \
           (test $$HDF5_Make_Ignore && echo "*** Error ignored") || \
           (cat $${log} && false) || exit 1; \
        echo "" | tee -a $${log}; \
        echo "Finished testing $${tname} $(TEST_FLAGS)" | tee -a $${log}; \
        echo "============================" | tee -a $${log}; \
        echo "Finished testing $${tname} $(TEST_FLAGS)";


