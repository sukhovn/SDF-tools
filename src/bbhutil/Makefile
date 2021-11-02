# Programs
CTOF = mfi
CC = gcc
YACC = bison
PERL = perl
RANLIB = ranlib
AR = ar

# Compiler Options
CFLAGS = -c -g -O2 -fcommon -fpic 
CFLAGS_NOOPT = -c  -fcommon -fpic
CPPFLAGS = -Iinclude   -I/usr/local/include  -DGFORTRAN43_OR_LATER -DLINUX -DPORT_GROUP -DWant_c_files -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_ATEXIT=1  -DLINUX -DF77_HAS_SYSTEM -DF77_HAS_CHDIR -DHAVE_LIBVS=1

CC_CMP = $(CC) $(CFLAGS) $(COPT) $(CPPFLAGS)
CC_CMP_NO_OPT = $(CC) $(CFLAGS_NOOPT) -g $(CPPFLAGS)
CC_PRE = $(CC) -E $(CPPFLAGS)

# System type for mfi
BBH_SYSTEM=LINUX

# BBHUTIL binaries
LIBBBHIO_OBJS = cliser.o sdf.o sdf_f.o gpar.tab.o lex.gpar.o
LIBBBHUTIL_OBJS = bbhutil.o bbhutil_f.o $(LIBBBHIO_OBJS)

all: dir $(LIBBBHUTIL_OBJS)

clean:
	rm -vf *.o *.i *.tab.c lex.*.c *.tab.h librnpl_f.c bbhutil_f.c sdf_f.c
	rm -rvf bin

dir:
	mkdir bin

cliser.o: cliser.c 
	$(CC_CMP) cliser.c

sdf.o: sdf.c
	$(CC_CMP_NO_OPT) sdf.c

sdf_f.i: sdf_f.h
	@/bin/cp sdf_f.h .sdf_f.c
	$(CC_PRE) .sdf_f.c > sdf_f.i
	@/bin/rm .sdf_f.c

sdf_f.c: sdf_f.i
	($(PERL) $(CTOF) sdf_f.i sdf_f.c $(BBH_SYSTEM) sdf)

sdf_f.o: sdf_f.c
	$(CC_CMP) sdf_f.c

gpar.tab.c: gpar.y gpar.h
	$(YACC) -b gpar -p gpar -d gpar.y

lex.gpar.c: gpar.l gpar.h
	$(LEX) -Pgpar gpar.l

lex.gpar.o: lex.gpar.c
	$(CC_CMP) lex.gpar.c

bbhutil.o: bbhutil.c
	$(CC_CMP) bbhutil.c

bbhutil_f.i: bbhutil_f.h
	@/bin/cp bbhutil_f.h .bbhutil_f.c
	$(CC_PRE) .bbhutil_f.c > bbhutil_f.i
	@/bin/rm .bbhutil_f.c
	
bbhutil_f.c: bbhutil_f.i
	($(PERL) $(CTOF) bbhutil_f.i bbhutil_f.c $(BBH_SYSTEM) bbhutil)
	
bbhutil_f.o: bbhutil_f.c
	$(CC_CMP) bbhutil_f.c