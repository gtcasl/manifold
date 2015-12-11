MAINTAINERCLEANFILES=  aclocal.m4 config.log config.status configure depcomp \
		      INSTALL install-sh Makefile.in missing

SUBDIRS = DRAMSim2-2.2.2
DRAMSim2lib_LIBRARIES = libDRAMSim2.a
DRAMSim2libdir = $(libdir)/manifold
libDRAMSim2_a_SOURCES = \
        dram_sim.cpp \
        dram_sim.h

pkginclude_DRAMSim2dir = $(includedir)/manifold/DRAMSim2

pkginclude_DRAMSim2_HEADERS = \
        dram_sim.h


libDRAMSim2_a_CPPFLAGS = -I$(KERNEL_INC)
