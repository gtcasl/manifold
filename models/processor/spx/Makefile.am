CXX = mpic++ -g -Wall -O3

MAINTAINERCLEANFILES=  aclocal.m4 config.log config.status configure COPYING depcomp \
		      INSTALL install-sh Makefile.in missing

spxlib_LIBRARIES = libspx.a
spxlibdir = $(libdir)/manifold

libspx_a_SOURCES = \
	instruction.cc \
	component.cc \
	pipeline.cc \
	cache/cache.cc \
	pipeline/outorder.cc \
	pipeline/inorder.cc \
	core.cc \
    qsim-proxy.cc

pkginclude_spxdir = $(includedir)/manifold/spx

pkginclude_spx_HEADERS = \
	instruction.h \
	component.h \
	pipeline.h \
	cache/cache.h \
	pipeline/outorder.h \
	pipeline/inorder.h \
	core.h \
    qsim-proxy.h

libspx_a_CPPFLAGS = -I$(KERNEL_INC) -I$(QSIM_INC) -Ipipeline -lqsim -lqsim-client
