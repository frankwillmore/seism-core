CXX=h5pcc
CXXFLAGS=-Wall -O2 -std=c++0x
#CXXFLAGS=-Wall -O2 -std=c++0x -DPRE_CREATE
#CXXFLAGS=-Wall -O0 -g -std=c++0x

all: seism-core

seism-core: seism-core.o
	$(CXX) $(CXXFLAGS) seism-core.o -o $@ -lstdc++

seism-core.o: seism-core.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o *.h5

veryclean: clean
	rm -f seism-core
