#!/usr/bin/make

INCLUDES=../insbin/hdf5/include
CXXFLAGS=-Wall -pedantic -O2 -std=c++0x -I$(INCLUDES)
CXXDEBUGFLAGS=-Wall -pedantic -O0 -g -std=c++0x -I$(INCLUDES)

all: seism-core-slice seism-core-check

seism-core-slice: seism-core-slice.o seism-core-attributes.o
	$(CXX) $(CXXFLAGS) seism-core-attributes.o seism-core-slice.o -o $@ -lstdc++

TEST_NAME=seism-core-slice
TEST_CMD=mpiexec -n 8 ./seism-core-slice < ./tests/check-0.in >> $${log}; \
        ./seism-core-check seism-test.h5 >> $${log}; \
        rm seism-test.h5 

check-slice: seism-core-slice seism-core-check
	tname=$(TEST_NAME); \
        log=$${tname}.chklog; \
        echo "============================" | tee $${log}; \
        echo "Testing $(HDF5_DRIVER) $${tname} $(TEST_FLAGS)"; \
        echo "$(HDF5_DRIVER) $${tname} $(TEST_FLAGS) Test Log" | tee -a $${log}; \
        echo "============================" | tee -a $${log}; \
        srcdir="$(srcdir)" \
           $(TEST_CMD) && touch $${tname}.chkexe || \
           (test $$HDF5_Make_Ignore && echo "*** Error ignored") || \
           (cat $${log} && false) || exit 1; \
        echo "" | tee -a $${log}; \
        echo "Finished testing $${tname} $(TEST_FLAGS)" | tee -a $${log}; \
        echo "============================" | tee -a $${log}; \
        echo "Finished testing $${tname} $(TEST_FLAGS)";

seism-core.o: src/seism-core.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

seism-core-slice.o: src/seism-core-slice.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

seism-core-attributes.o: src/seism-core-attributes.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

seism-core-check: src/seism-core-check.cc seism-core-attributes.o
	$(CXX) $(CXXFLAGS) src/seism-core-check.cc seism-core-attributes.o -o seism-core-check -lstdc++

seism-core-check-debug: src/seism-core-check.cc seism-core-attributes.o
	$(CXX) $(CXXDEBUGFLAGS) src/seism-core-check.cc seism-core-attributes.o -o seism-core-check -lstdc++

clean:
	rm -f *.o *.h5

veryclean: clean
	rm -f seism-core seism-core-slice seism-core-slicexx seism-core-check *.chkexe *.chklog


check: check-slice
