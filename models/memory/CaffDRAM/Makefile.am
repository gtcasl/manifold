MAINTAINERCLEANFILES=  aclocal.m4 config.log config.status configure depcomp \
		      INSTALL install-sh Makefile.in missing

caffdram_lib_LIBRARIES = libcaffdram.a
caffdram_libdir = $(libdir)/manifold
libcaffdram_a_SOURCES = \
	Bank.cpp \
	Bank.h \
	Channel.cpp \
	Channel.h \
	Controller.cpp \
	Controller.h \
	ControllerSimple.cpp \
	ControllerSimple.h \
	Dreq.cpp \
	Dreq.h \
	Dsettings.cpp \
	Dsettings.h \
	McMap.cpp \
	McMap.h \
	Rank.cpp \
	Rank.h

pkginclude_caffdramdir = $(includedir)/manifold/caffdram

pkginclude_caffdram_HEADERS = \
	McMap.h \
	Controller.h \
	ControllerSimple.h

libcaffdram_a_CPPFLAGS = -I$(KERNEL_INC)
