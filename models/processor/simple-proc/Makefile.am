CXX = mpic++ -Wall

MAINTAINERCLEANFILES=  aclocal.m4 config.log config.status configure COPYING depcomp \
		      INSTALL install-sh Makefile.in missing

simple_proclib_LIBRARIES = libsimple-proc.a
simple_proclibdir = $(libdir)/manifold
libsimple_proc_a_SOURCES = \
	instruction.cc \
	instruction.h \
	qsim-proc.cc \
	qsim-proc.h \
	qsimlib-proc.cc \
	qsimlib-proc.h \
	simple-proc.cc \
	simple-proc.h \
	trace-proc.cc \
	trace-proc.h

pkginclude_simple_procdir = $(includedir)/manifold/simple-proc

pkginclude_simple_proc_HEADERS = \
	instruction.h \
	qsimlib-proc.h \
	simple-proc.h \
	trace-proc.h


libsimple_proc_a_CPPFLAGS = -I$(KERNEL_INC) -I$(QSIM_INC)
