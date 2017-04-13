MAINTAINERCLEANFILES=  aclocal.m4 config.log config.status configure depcomp \
		      INSTALL install-sh Makefile.in missing

pagevault_lib_LIBRARIES = libpagevault.a
pagevault_libdir = $(libdir)/manifold
libpagevault_a_SOURCES = \
    utils.cpp \
    controller.cpp \
	dramsim2.cpp \
    hmcsim.cpp \
	eacore.cpp \
	gmt.cpp \
	bmt.cpp \
	sgx.cpp \
	pmt.cpp
    

pkginclude_pagevaultdir = $(includedir)/manifold/PageVault

pkginclude_pagevault_HEADERS = \
    utils.h	 \
    controller.h \
    dramsim2.h \
	hmcsim.h \
    cache.h \
	eacore.h \
	gmt.h \
	bmt.h \
	sgx.h \
	pmt.h    

libpagevault_a_CPPFLAGS = -I$(KERNEL_INC) -I../../../../hmcsim/include

libpagevault.a: $(libpagevault_a_OBJECTS)
	-rm -f $@
	cp ../../../../hmcsim/libhmcsim.a $@
	$(AR) r $@ $(libpagevault_a_OBJECTS)
	$(RANLIB) $@

AM_CXXFLAGS=-std=c++11 -g -O0
