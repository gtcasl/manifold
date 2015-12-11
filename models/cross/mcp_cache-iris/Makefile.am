MAINTAINERCLEANFILES=  aclocal.m4 config.log config.status configure COPYING depcomp \
		      INSTALL install-sh Makefile.in missing

mcp_irislib_LIBRARIES = libmcp-iris.a
mcp_irislibdir = $(libdir)/manifold
libmcp_iris_a_SOURCES = \
	mcp-iris.cc \
	mcp-iris.h

pkginclude_mcp_irisdir = $(includedir)/manifold/mcp-iris

pkginclude_mcp_iris_HEADERS = \
	mcp-iris.h

libmcp_iris_a_CPPFLAGS = -I$(KERNEL_INC) -I$(UARCH_INC) -I$(IRIS_INC) -I$(MCP_INC)
