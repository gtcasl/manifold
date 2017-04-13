AC_PREREQ([2.59])
AC_INIT([manifold], [0.13])
AM_INIT_AUTOMAKE([-Wall -Werror])
#LT_PREREQ([2.2])
#LT_INIT

AC_PROG_CXX
AC_PROG_RANLIB


AC_CONFIG_SUBDIRS([kernel
                   uarch
                   models/cache/mcp-cache
                   models/cache/simple-cache
                   models/memory/PageVault
                   models/memory/CaffDRAM
                   models/memory/DRAMSim2
                   models/network/iris
                   models/processor/zesto
                   models/processor/simple-proc
                   models/processor/spx
                   models/cross/mcp_cache-iris
                   models/qsim/interrupt_handler
                   models/qsim/proxy
                   models/kitfox/proxy
           ])

#models/memory/simple-mc
#models/network/simple-net

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
