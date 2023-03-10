EXECUTABLES=lifegame lifegame_mpi

EXPENSIVE_JUNK += $(EXECUTABLES)

SRC = lifegame.c lifegame_mpi.c

JUNK +=

CFLAGS += -O3 -Wall -W --std=c11 -lm
CXXFLAGS += -O3 -Wall -W --std=c++11 -lm -Wno-cast-function-type
OMP_CFLAGS = $(CFLAGS) -fopenmp
MPI_CFLAGS = $(CXXFLAGS) -lmpi

help:
	@echo "help\tShow this help text"
	@echo "all\tMake all executables"
	@echo "clean\tThrow away all files that are easy to produce again"
	@echo "empty\tThrow away all files that can be produced again"

all: $(EXECUTABLES)

clean:
	rm -rf $(JUNK)

empty:
	rm -rf $(JUNK) $(EXPENSIVE_JUNK)

lifegame: lifegame.c
	$(CC) $(OMP_CFLAGS) -o lifegame lifegame.c
lifegame_mpi: lifegame_mpi.c
	mpiCC $(MPI_CFLAGS) -o lifegame_mpi lifegame_mpi.c

