#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([iris],[0.1],[mitchelle dot rasquinha at gatech.edu])
AM_INIT_AUTOMAKE([manifold_iris], [1.0], [mitch])
AC_CONFIG_SRCDIR([interfaces/genericHeader.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB
AC_CHECK_PROGS(MPICXX,$CC)

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lc':
AC_CHECK_LIB([c], [main])
# FIXME: Replace `main' with a function in `-lcppunit':
#AC_CHECK_LIB([cppunit], [main])
# FIXME: Replace `main' with a function in `-ldl':
AC_CHECK_LIB([dl], [main])
# FIXME: Replace `main' with a function in `-lgcov':
AC_CHECK_LIB([gcov], [main])
# FIXME: Replace `main' with a function in `-lgsl':
AC_CHECK_LIB([gsl], [main])
# FIXME: Replace `main' with a function in `-lgslcblas':
AC_CHECK_LIB([gslcblas], [main])
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [main])
# FIXME: Replace `main' with a function in `-lmanifold':
AC_CHECK_LIB([manifold], [main])
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
#AC_C_INLINE
#AC_TYPE_SIZE_T
#AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.


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


# enable stats by default
AC_ARG_ENABLE([stats],
    [AS_HELP_STRING([--disable-stats],
        [disable statistics collection @<:@default: no@:>@])],
    [stats=${enableval}], [stats=yes])

if test "x${para_sim}" = xyes ; then
    AC_DEFINE(STATS)
else # stats disabled
    AC_MSG_WARN([
    ---------------
    Stats disabled.
    ---------------])
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


# disable FORECAST_NULL by default
AC_ARG_ENABLE([forecast_null],
    [AS_HELP_STRING([--enable-forecast-null],
        [enable Forecat Null Message Algorithm @<:@default: no@:>@])],
    [forecast_null=${enableval}], [forecast_null=no])

if test "x${forecast_null}" = xyes ; then
    AC_MSG_WARN([
    ----------------------
    FORECAST_NULL enabled!
    ----------------------])
    AC_DEFINE(FORECAST_NULL)
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
