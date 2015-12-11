CXX = mpic++

MAINTAINERCLEANFILES=  aclocal.m4 config.log config.status configure COPYING depcomp \
		      INSTALL install-sh Makefile.in missing

simple_cachelib_LIBRARIES = libsimple-cache.a
simple_cachelibdir = $(libdir)/manifold
libsimple_cache_a_SOURCES = \
	cache_req.cpp \
	cache_req.h \
	hash_table.cpp \
	hash_table.h \
	simple_cache.cpp \
	simple_cache.h

pkginclude_simple_cachedir = $(includedir)/manifold/simple-cache

pkginclude_simple_cache_HEADERS = \
	cache_req.h \
	simple_cache.h

libsimple_cache_a_CPPFLAGS = -I$(KERNEL_INC)
