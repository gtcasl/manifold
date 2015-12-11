#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([manifold-kernel], [1.0])
AM_INIT_AUTOMAKE([manifold_kernel], [1.0])

AC_CONFIG_SRCDIR([manifold.cc])
#AC_CONFIG_HEADERS([config.h])

AC_PROG_CXX

# Checks for programs.

# Checks for command-line options

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


# disable large-size data by default
AC_ARG_ENABLE([kernel_large_data],
    [AS_HELP_STRING([--enable-kernel-large-data],
        [enable data between components to be any (unknown) size @<:@default: no@:>@])],
    [kernel_large_data=${enableval}], [kernel_large_data=no])

if test "x${kernel_large_data}" = xyes ; then
    AC_DEFINE(KERNEL_ANY_DATA_SIZE)
    AC_MSG_WARN([
    -----------------------------
    Enabling arbitrary data size!
    -----------------------------])
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



AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
