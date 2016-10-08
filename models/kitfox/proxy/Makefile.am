CXX = mpic++ -g -Wall -O3

MAINTAINERCLEANFILES=aclocal.m4 config.log config.status configure COPYING depcomp \
					INSTALL install-sh Makefile.in missing

kitfox_proxylib_LIBRARIES = libkitfox_proxy.a
kitfox_proxylibdir = $(libdir)/manifold

libkitfox_proxy_a_SOURCES = \
	kitfox_proxy.cc

pkginclude_kitfox_proxydir = $(includedir)/manifold/proxy

pkginclude_kitfox_proxy_HEADERS = \
	kitfox_proxy.h

libkitfox_proxy_a_CPPFLAGS = -I$(KERNEL_INC) -I$(KITFOX_INC) -lkitfox
