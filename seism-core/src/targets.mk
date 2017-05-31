
seism-core-slice: seism-core-slice.o seism-core-attributes.o
	$(CXX) $(CXXFLAGS) $^ -o $@ -ldl

check-slice: seism-core-slice seism-core-check
	@printf "\033[31;01m *** generating test file for 2x2x2 domain ***\033[0m\n"
	$(MPI_LAUNCHER) -n 8 ./seism-core-slice < ../tests/check.in
	@printf "\033[31;01m *** test file created ***\033[0m\n"
	./seism-core-check seism-test.h5
	@printf "\033[31;01m *** check complete ***\033[0m\n\n"
	@printf "\033[31;01m *** removing test file ***\033[0m\n\n"
	rm seism-test.h5
	@printf "\033[31;01m *** done ***\033[0m\n\n"

check-subfile: seism-core-slice seism-read
	$(MPI_LAUNCHER) -n 8 ./seism-core-slice < ../tests/subfile.in
	$(MPI_LAUNCHER) -n 8 ./seism-read seism-test.h5
	$(MPI_LAUNCHER) -n 8 ./seism-read seism-test.h5 --ignore-subfile
	rm -f seism-test.h5 Subfile_*.h5

check-read: seism-core-slice seism-read 
	$(MPI_LAUNCHER) -n 8 ./seism-core-slice < ../tests/check.in
	$(MPI_LAUNCHER) -n 8 ./seism-read seism-test.h5
	rm -f seism-test.h5 Subfile_*.h5

seism-read: seism-read.o seism-core-attributes.o 
	$(CXX) $(CXXFLAGS) $^ -o $@ -ldl 

check-serial: seism-core-slice seism-core-check
	./seism-core-slice < ../tests/serial.in
	./seism-core-check seism-test.h5
	rm seism-test.h5

check-plugin: seism-core-slice seism-core-check seism-core-plugin
	LD_LIBRARY_PATH=`pwd`/plugins $(MPI_LAUNCHER) -n 8 ./seism-core-slice < ../tests/check-plugin.in
	rm seism-test.h5


check-sd: seism-core-slice seism-core-check seism-core-plugin
	LD_LIBRARY_PATH=`pwd`/plugins $(MPI_LAUNCHER) -n 8 ./seism-core-slice < ../tests/check-sd.in
	rm seism-test.h5

#--- executables

seism-core-check: seism-core-check.cc seism-core-attributes.o
	$(CXX) $(CXXFLAGS) $^ -o $@ 

seism-core-plugin: plugins/libplugins.so 
	cd plugins && make 

seism-core-check-debug: seism-core-check.cc seism-core-attributes.o
	$(CXX) $(CXXDEBUGFLAGS) $^ -o $@

#--- object files

seism-read.o: seism-read.cc
	$(CXX) $(CXXFLAGS) -c $^ -o $@

seism-core-slice.o: seism-core-slice.cc
	$(CXX) $(CXXFLAGS) -c $^ -o $@

seism-core-attributes.o: seism-core-attributes.cc
	$(CXX) $(CXXFLAGS) -c $^ -o $@

#--- other PHONY targets

clean:
	rm -f *.o *.h5

veryclean: clean
	rm -f seism-core-slice seism-core-check seism-read 




