CXX = mpic++ -g -Wall -O3

MAINTAINERCLEANFILES=aclocal.m4 config.log config.status configure COPYING depcomp \
					INSTALL install-sh Makefile.in missing

qsim_proxylib_LIBRARIES = libqsim_proxy.a
qsim_proxylibdir = $(libdir)/manifold

libqsim_proxy_a_SOURCES = \
	proxy.cc

pkginclude_qsim_proxydir = $(includedir)/manifold/proxy

pkginclude_qsim_proxy_HEADERS = \
	proxy.h

libqsim_proxy_a_CPPFLAGS = -I$(KERNEL_INC) -I$(QSIM_INC) -lqsim -lqsim-client
