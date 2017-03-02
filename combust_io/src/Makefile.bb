
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
	rm -f *.x *~

check:
	sh ./scripts/kernel.sh
