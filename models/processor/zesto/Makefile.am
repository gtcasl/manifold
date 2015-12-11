# COPYING is not deleted because it's from simplescalar, not autoconf.
MAINTAINERCLEANFILES=  aclocal.m4 compile config.log config.status configure depcomp \
		      INSTALL install-sh Makefile.in missing

Zestolib_LIBRARIES = libZesto.a
Zestolibdir = $(libdir)/manifold
libZesto_a_SOURCES = \
	elf.h \
	endian.c \
	endian.h \
	eval.c \
	eval.h \
	host.h \
	loader.h \
	machine.c \
	machine.h \
	memory.c \
	memory.h \
	misc.c \
	misc.h \
	options.c \
	options.h \
	range.c \
	range.h \
	regs.c \
	regs.h \
	sim.h \
	stats.c \
	stats.h \
	symbol.c \
	symbol.h \
	thread.h \
	version.h \
	valcheck.h \
	machine.def \
	x86flow.def \
	\
	zesto-alloc.cpp \
	zesto-alloc.h \
	zesto-bpred.cpp \
	zesto-bpred.h \
	zesto-commit.cpp \
	zesto-commit.h \
	zesto-core.cpp \
	zesto-core.h \
	qsimclient-core.cc \
	qsimclient-core.h \
	qsimlib-core.cc \
	qsimlib-core.h \
	qsimproxy-core.cc \
	qsimproxy-core.h \
	shm.cc \
	shm.h \
	trace-core.cc \
	trace-core.h \
	zesto-decode.cpp \
	zesto-decode.h \
	zesto-exec.cpp \
	zesto-exec.h \
	zesto-fetch.cpp \
	zesto-fetch.h \
	zesto-memdep.cpp \
	zesto-memdep.h \
	zesto-opts.c \
	zesto-opts.h \
	zesto-oracle.cpp \
	zesto-oracle.h \
	zesto-structs.h \
	\
	ZPIPE-alloc.list \
	ZPIPE-commit.list \
	ZPIPE-decode.list \
	ZPIPE-exec.list \
	ZPIPE-fetch.list

#files that should be distributed
# Files under ZCOMPS-xxx probably don't need to be distributed.
EXTRA_DIST = COPYING \
	ZPIPE-alloc/alloc-atom.cpp \
	ZPIPE-alloc/alloc-DPM.cpp \
	ZPIPE-commit/commit-atom.cpp \
	ZPIPE-commit/commit-DPM.cpp \
	ZPIPE-decode/decode-atom.cpp \
	ZPIPE-decode/decode-DPM.cpp \
	ZPIPE-exec/exec-atom.cpp \
	ZPIPE-exec/exec-DPM.cpp \
	ZPIPE-fetch/fetch-atom.cpp \
	ZPIPE-fetch/fetch-DPM.cpp \
	ZPIPE-fetch/fetch-DPM.h \
	\
	ZCOMPS-bpred.list \
	ZCOMPS-bpred/bpred-2bc.cpp \
	ZCOMPS-bpred/bpred-2lev.cpp \
	ZCOMPS-bpred/bpred-alloyedperceptron.cpp \
	ZCOMPS-bpred/bpred-bimode.cpp \
	ZCOMPS-bpred/bpred-blg.cpp \
	ZCOMPS-bpred/bpred-btfnt.cpp \
	ZCOMPS-bpred/bpred-nottaken.cpp \
	ZCOMPS-bpred/bpred-pathneural.cpp \
	ZCOMPS-bpred/bpred-perceptron.cpp \
	ZCOMPS-bpred/bpred-perfect.cpp \
	ZCOMPS-bpred/bpred-pwl.cpp \
	ZCOMPS-bpred/bpred-random.cpp \
	ZCOMPS-bpred/bpred-skewed.cpp \
	ZCOMPS-bpred/bpred-tage.cpp \
	ZCOMPS-bpred/bpred-taken.cpp \
	ZCOMPS-bpred/bpred-yags.cpp \
	ZCOMPS-btb.list \
	ZCOMPS-btb/btb-2levbtac.cpp \
	ZCOMPS-btb/btb-btac.cpp \
	ZCOMPS-btb/btb-perfect.cpp \
	ZCOMPS-dram.list \
	ZCOMPS-dram/dram-simplescalar.cpp \
	ZCOMPS-dram/dram-simplesdram.cpp \
	ZCOMPS-fusion.list \
	ZCOMPS-fusion/fusion-priority.cpp \
	ZCOMPS-fusion/fusion-multihybrid.cpp \
	ZCOMPS-fusion/fusion-singleton.cpp \
	ZCOMPS-fusion/fusion-colt.cpp \
	ZCOMPS-fusion/fusion-random.cpp \
	ZCOMPS-fusion/fusion-mcfarling.cpp \
	ZCOMPS-fusion/fusion-oracle.cpp \
	ZCOMPS-MC.list \
	ZCOMPS-MC/MC-simple.cpp \
	ZCOMPS-memdep.list \
	ZCOMPS-memdep/memdep-blind.cpp \
	ZCOMPS-memdep/memdep-lwt.cpp \
	ZCOMPS-memdep/memdep-none.cpp \
	ZCOMPS-memdep/memdep-oracle.cpp \
	\
	ZCOMPS-prefetch.list \
	ZCOMPS-prefetch/prefetch-context.cpp \
	ZCOMPS-prefetch/prefetch-IP.cpp \
	ZCOMPS-prefetch/prefetch-nextline.cpp \
	ZCOMPS-prefetch/prefetch-stream.cpp \
	ZCOMPS-ras.list \
	ZCOMPS-ras/ras-perfect.cpp \
	ZCOMPS-ras/ras-stack.cpp \
	\
	doc \
	proxy_1 proxy_mt

pkginclude_Zestodir = $(includedir)/manifold/Zesto

pkginclude_Zesto_HEADERS = \
	qsimclient-core.h \
	qsimlib-core.h \
	qsimproxy-core.h \
	shm.h \
	trace-core.h \
	zesto-core.h


libZesto_a_CPPFLAGS = -DMIN_SYSCALL_MODE -DUSE_SSE_MOVE -msse4a -mfpmath=sse -I$(KERNEL_INC)  -I$(QSIM_INC) -O0


dist-hook:
	find $(distdir)/doc -name '.svn' | xargs rm -rf
	find $(distdir)/proxy_1 -name '.svn' | xargs rm -rf
	find $(distdir)/proxy_mt -name '.svn' | xargs rm -rf
