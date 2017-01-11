CXX = mpic++ -g -Wall -O3

MAINTAINERCLEANFILES=aclocal.m4 config.log config.status configure COPYING depcomp \
					INSTALL install-sh Makefile.in missing

qsim_interrupt_handlerlib_LIBRARIES = libqsim_interrupt_handler.a
qsim_interrupt_handlerlibdir = $(libdir)/manifold

libqsim_interrupt_handler_a_SOURCES = \
	interrupt_handler.cc

pkginclude_qsim_interrupt_handlerdir = $(includedir)/manifold/interrupt_handler

pkginclude_qsim_interrupt_handler_HEADERS = \
	interrupt_handler.h

libqsim_interrupt_handler_a_CPPFLAGS = -I$(KERNEL_INC) -I$(QSIM_INC) -lqsim -lqsim-client
