CXX=h5pcc
CXXFLAGS=-Wall -O2

all: seism-core

seism-core: seism-core.o parse_input.o
	$(CXX) seism-core.o parse_input.o -o $@ -lstdc++

parse_input.o: parse_input.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ 

seism-core.o: seism-core.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o *.h5

veryclean: clean
	rm -f seism-core
