#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE([manifold_mcp-iris], [1.0])
AC_CONFIG_SRCDIR([mcp-iris.h])
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.


# enable parallel simulation by default
AC_ARG_ENABLE([para_sim],
    [AS_HELP_STRING([--disable-para-sim],
        [disable parallel simulation @<:@default: no@:>@])],
    [para_sim=${enableval}], [para_sim=yes])

if test "x${para_sim}" = xyes ; then
    AC_CHECK_PROGS([MPICPP], [mpic++], [mpic++])
    if test "${MPICPP}" = "mpic++" ; then # para_sim enabled; mpic++ found
        AC_SUBST(CXX, mpic++)
    else # para_sim enabled; mpic++ not found
        AC_MSG_WARN([
	---------------------------------------------
	mpic++ not found! Building serial simulation!
	---------------------------------------------])
	AC_DEFINE(NO_MPI)
    fi
else # para_sim disabled
    AC_DEFINE(NO_MPI)
    AC_MSG_WARN([
    ---------------------------------------------------------
    Parallel simulation disabled! Building serial simulation!
    ---------------------------------------------------------])
fi


# configure variables: KERINC
# This can be used to let user tell us where to find kernel headers.
# For example: configure  KERINC="foo/include/kernel"
AC_ARG_VAR([KERINC], [The path where kernel headers can be found.])

# if KERINC not specified, first look in super-project, and if not found,
# then set to installation directory.
if test -z "$KERINC" ; then
    if test -d "$srcdir/../../../kernel"; then
	KERINC='$(top_srcdir)/../../..'
	AC_MSG_NOTICE([
    -----------------------------------------
    Use kernel header files in super-project.
    -----------------------------------------
	])
    else
	#check installation dir
	# unfortunately I can't use if test -f "$includedir/manifold/kernel/manifold.h"
	# because $includedir will only expand to ${prefix}/include

	# simply set the value. I can't check if it's really there.
	KERINC='$(includedir)/manifold'
	AC_MSG_NOTICE([
    --------------------------------------------------
    Use kernel header files in installation direcotry.
    --------------------------------------------------
	])
    fi
else # KERINC is specified
    AC_MSG_NOTICE([
    -----------------------------------------
    Use kernel header files in $KERINC
    -----------------------------------------
    ])
fi

AC_SUBST([KERNEL_INC], [$KERINC])



# configure variables: UARCHINC
# This can be used to let user tell us where to find kernel headers.
# For example: configure  UARCHINC="foo/include/uarch"
AC_ARG_VAR([UARCHINC], [The path where uarch headers can be found.])

# if UARCHINC not specified, first look in super-project, and if not found,
# then set to installation directory.
if test -z "$UARCHINC" ; then
    if test -d "$srcdir/../../../uarch"; then
	UARCHINC='$(top_srcdir)/../../..'
	AC_MSG_NOTICE([
    ----------------------------------------
    Use uarch header files in super-project.
    ----------------------------------------
	])
    else
	#check installation dir
	# unfortunately I can't use if test -f "$includedir/manifold/uarch/foo.h"
	# because $includedir will only expand to ${prefix}/include

	# simply set the value. I can't check if it's really there.
	UARCHINC='$(includedir)/manifold'
	AC_MSG_NOTICE([
    -------------------------------------------------
    Use uarch header files in installation direcotry.
    -------------------------------------------------
	])
    fi
else # UARCHINC is specified
    AC_MSG_NOTICE([
    -----------------------------------
    Use uarch header files in $UARCHINC
    -----------------------------------
    ])
fi

AC_SUBST([UARCH_INC], [$UARCHINC])


# configure variables: MCPINC
# This can be used to let user tell us where to find mcp-cache headers.
# For example: configure  MCPINC="foo/include/mcp-cache"
AC_ARG_VAR([MCPINC], [The path where mcp-cache headers can be found.])

# if MCPINC not specified, first look in super-project, and if not found,
# then set to installation directory.
if test -z "$MCPINC" ; then
    if test -d "$srcdir/../../../models/cache/mcp-cache"; then
	MCPINC='$(top_srcdir)/../../../models/cache'
	AC_MSG_NOTICE([
    --------------------------------------------
    Use mcp-cache header files in super-project.
    --------------------------------------------
	])
    else
	#check installation dir
	# unfortunately I can't use if test -f "$includedir/manifold/mcp-cache/foo.h"
	# because $includedir will only expand to ${prefix}/include

	# simply set the value. I can't check if it's really there.
	MCPINC='$(includedir)'
	AC_MSG_NOTICE([
    -----------------------------------------------------
    Use mcp-cache header files in installation direcotry.
    -----------------------------------------------------
	])
    fi
else # MCPINC is specified
    AC_MSG_NOTICE([
    ----------------------------------
    Use kernel header files in $MCPINC
    ----------------------------------
    ])
fi

AC_SUBST([MCP_INC], [$MCPINC])



# configure variables: IRISINC
# This can be used to let user tell us where to find Iris headers.
# For example: configure  IRISINC="foo/include/iris"
AC_ARG_VAR([IRISINC], [The path where Iris headers can be found.])

# if IRISINC not specified, first look in super-project, and if not found,
# then set to installation directory.
if test -z "$IRISINC" ; then
    if test -d "$srcdir/../../../models/network/iris"; then
	IRISINC='$(top_srcdir)/../../../models/network'
	AC_MSG_NOTICE([
    ---------------------------------------
    Use Iris header files in super-project.
    ---------------------------------------
	])
    else
	#check installation dir
	# unfortunately I can't use if test -f "$includedir/manifold/iris/interfaces/simulatedLen.h"
	# because $includedir will only expand to ${prefix}/include

	# simply set the value. I can't check if it's really there.
	IRISINC='$(includedir)'
	AC_MSG_NOTICE([
    ------------------------------------------------
    Use Iris header files in installation direcotry.
    ------------------------------------------------
	])
    fi
else # IRISINC is specified
    AC_MSG_NOTICE([
    -----------------------------------------
    Use kernel header files in $IRISINC
    -----------------------------------------
    ])
fi

AC_SUBST([IRIS_INC], [$IRISINC])


# Checks for header files.
#AC_CHECK_HEADERS([string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
