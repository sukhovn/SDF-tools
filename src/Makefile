.PHONY: all clean

CC = gcc # compiler to use
CFLAGS = -c -O3 -Ibbhutil # flags to use at compliation
LINKERFLAG = -lm -fpic

# BBHUTIL binaries
LIBBBHIO_OBJS = cliser.o sdf.o sdf_f.o gpar.tab.o lex.gpar.o
LIBBBHUTIL_OBJS = bbhutil.o bbhutil_f.o $(LIBBBHIO_OBJS)
LIBBBHUTIL_BINS = $(patsubst %, bbhutil/%, $(LIBBBHUTIL_OBJS))

all: routines_lib.o 
	(cd bbhutil; make)
	${CC} -shared -o routines_lib.so routines_lib.o $(LIBBBHUTIL_BINS) ${LINKERFLAG}

routines_lib.o : routines_lib.c
	@echo "Compiling routines_lib.c..."
	${CC} ${CFLAGS} -fpic -o routines_lib.o  routines_lib.c

clean:
	@echo "Cleaning up..."
	(cd bbhutil; make clean)
	rm -rvf *.o
	rm -v routines_lib.so