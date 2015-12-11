MAINTAINERCLEANFILES=  aclocal.m4 config.log config.status configure depcomp \
		      INSTALL install-sh Makefile.in missing

mcp_cachelib_LIBRARIES = libmcp-cache.a
mcp_cachelibdir = $(libdir)/manifold
libmcp_cache_a_SOURCES = \
	cache_req.cpp \
	cache_req.h \
	cache_types.h \
	coh_mem_req.cpp \
	coh_mem_req.h \
	debug.h \
	hash_table.cpp \
	hash_table.h \
	L1_cache.cpp \
	L1_cache.h \
	L2_cache.cpp \
	L2_cache.h \
	LLP_cache.cpp \
	LLP_cache.h \
	LLS_cache.cpp \
	LLS_cache.h \
	MESI_L1_cache.cpp \
	MESI_L1_cache.h \
	MESI_L2_cache.cpp \
	MESI_L2_cache.h \
	MESI_LLP_cache.cpp \
	MESI_LLP_cache.h \
	MESI_LLS_cache.cpp \
	MESI_LLS_cache.h \
	mux_demux.cpp \
	mux_demux.h \
	lp_lls_unit.cpp \
	lp_lls_unit.h \
	\
	coherence/ClientInterface.cpp \
	coherence/ClientInterface.h \
	coherence/ManagerInterface.cpp \
	coherence/ManagerInterface.h \
	coherence/MESI_client.cpp \
	coherence/MESI_client.h \
	coherence/MESI_enum.h \
	coherence/MESI_manager.cpp \
	coherence/MESI_manager.h \
	coherence/sharers.cpp \
	coherence/sharers.h

pkginclude_mcp_cachedir = $(includedir)/manifold/mcp-cache

pkginclude_mcp_cache_HEADERS = \
	cache_req.h \
	cache_types.h \
	coh_mem_req.h \
	debug.h \
	L1_cache.h \
	L2_cache.h \
	MESI_L1_cache.h \
	MESI_L2_cache.h \
	MESI_LLP_cache.h \
	MESI_LLS_cache.h \
	lp_lls_unit.h \
	mux_demux.h

libmcp_cache_a_CPPFLAGS = -I$(KERNEL_INC)

EXTRA_DIST = doc

dist-hook:
	find $(distdir)/doc -name '.svn' | xargs rm -rf
