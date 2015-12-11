MAINTAINERCLEANFILES=  aclocal.m4 config.log config.status configure depcomp \
		      INSTALL install-sh Makefile.in missing

manifoldlib_LIBRARIES = libmanifold.a
manifoldlibdir = $(libdir)/manifold
libmanifold_a_SOURCES = \
	clock.cc \
	clock.h \
	common-defs.h \
	component.cc \
	component-decl.h \
	component.h \
	link.cc \
	link-decl.h \
	link.h \
  lookahead.cc \
  lookahead.h \
	manifold.cc \
	manifold-decl.h \
	manifold-event.h \
	manifold.h \
	message.h \
	messenger.cc \
	messenger.h \
	quantum_scheduler.cc \
	quantum_scheduler.h \
	scheduler.cc \
	scheduler.h \
	serialize.h \
	stat.h \
	stat_engine.cc \
	stat_engine.h \
  syncalg.cc \
  syncalg.h

manifoldkernelincdir = $(includedir)/manifold/kernel
manifoldkernelinc_HEADERS = \
	clock.h \
	common-defs.h \
	component-decl.h \
	component.h \
	link-decl.h \
	link.h \
  lookahead.h \
	manifold-decl.h \
	manifold-event.h \
	manifold.h \
	quantum_scheduler.h \
	scheduler.h \
	serialize.h \
	stat.h \
	stat_engine.h

EXTRA_DIST = doc

dist-hook:
	find $(distdir)/doc -name '.svn' | xargs rm -rf

