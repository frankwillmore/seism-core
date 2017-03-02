
# Should not need to modify anything below...

FFLAGS=-O2

all: kernel

kernel: KERNEL.o
	$(FC) IO.o KERNEL.o -o kernel.x

KERNEL.o: IO.o KERNEL.F90
	$(FC) $(FFLAGS) -c KERNEL.F90

IO.o: IO.F90
	$(FC) $(FFLAGS) -c IO.F90

clean:
	rm -f *.o *.mod

veryclean: clean
	rm -f *.x *~

check:
	sh ./kernel.sh
