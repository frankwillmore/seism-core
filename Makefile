CXX=h5pcc
CXXFLAGS=-Wall -O2

all: seism-core

seism-core: seism-core.o
	$(CXX) seism-core.o -o $@ -lstdc++

seism-core.o: seism-core.cc
	$(CXX) $(CXXLAGS) -c $< -o $@

clean:
	rm -f *.o *.h5

veryclean: clean
	rm -f seism-core
