MAINTAINERCLEANFILES=  aclocal.m4 config.log config.status configure depcomp \
		      INSTALL install-sh Makefile.in missing

irislib_LIBRARIES = libiris.a
irislibdir = $(libdir)/manifold
libiris_a_SOURCES = \
	    interfaces/genericHeader.h \
	    interfaces/genericIrisInterface.h \
	    interfaces/mapping.h \
	    interfaces/mapping.cc \
	    interfaces/simulatedLen.h \
	    interfaces/vnetAssign.h \
	    \
	    data_types/flit.cc \
	    data_types/flit.h \
	    data_types/linkData.cc \
	    data_types/linkData.h \
	    \
	    components/CrossBarSwitch.cc \
	    components/CrossBarSwitch.h \
	    components/genericBuffer.cc \
	    components/genericBuffer.h \
	    components/genericRC.cc \
	    components/genericRC.h \
	    components/genericSwitchArbiter.cc \
	    components/genericSwitchArbiter.h \
	    components/genericVcAllocator.cc \
	    components/genericVcAllocator.h \
	    components/genOneVcIrisInterface.h \
	    components/simpleArbiter.cc \
	    components/simpleArbiter.h \
	    components/simpleRouter.cc \
	    components/simpleRouter.h

pkginclude_interfacesdir = $(includedir)/manifold/iris
pkginclude_iris_componentsdir = $(includedir)/manifold/iris/components
pkginclude_iris_data_typesdir = $(includedir)/manifold/iris/data_types
pkginclude_iris_topologydir = $(includedir)/manifold/iris/topology
pkginclude_iris_genericTopologydir = $(includedir)/manifold/iris/genericTopology

pkginclude_interfaces_HEADERS = \
	    interfaces/genericHeader.h \
	    interfaces/genericIrisInterface.h \
	    interfaces/mapping.h \
	    interfaces/simulatedLen.h \
	    interfaces/vnetAssign.h

pkginclude_iris_components_HEADERS = \
	    components/CrossBarSwitch.h \
	    components/genOneVcIrisInterface.h \
	    components/simpleRouter.h

pkginclude_iris_data_types_HEADERS = \
	    data_types/flit.h \
	    data_types/linkData.h

pkginclude_iris_genericTopology_HEADERS = \
            genericTopology/CrossBar.h \
            genericTopology/genericTopoCreator.h \
	    genericTopology/ring.h \
	    genericTopology/torus.h

###libIris
libiris_a_CPPFLAGS = -I. -I$(KERNEL_INC)

EXTRA_DIST = doc

dist-hook:
	find $(distdir)/doc -name '.svn' | xargs rm -rf

