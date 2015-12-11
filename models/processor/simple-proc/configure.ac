#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE([manifold_simple_proc], [1.0])
AC_CONFIG_SRCDIR([simple-proc.h])
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
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



# use QSim by default
AC_ARG_WITH([qsim],
    [AS_HELP_STRING([--without-qsim],
        [do not use QSim @<:@default: no@:>@])],
    [qsim=${enableval}], [qsim=yes])

if test "x${qsim}" = xyes ; then
    #check if QSim header files exist
    # We allow user to specify the location of the header files, e.g.,
    # configure QSIMINC=/foo/qsim
    if test -z "$QSIMINC" ; then
	QSIMINC='/usr/local/include'
    fi

    if test ! -e "$QSIMINC/qsim.h"; then
	AC_MSG_ERROR([
    -----------------------------------------
    ERROR: qsim.h not found in $QSIMINC!
    -----------------------------------------
	])
    else
	AC_DEFINE(USE_QSIM)
	AC_SUBST([QSIM_INC], [$QSIMINC])
    fi
else # use_qsim disabled
    AC_MSG_WARN([
    -----------------------------------------------------------------
    USE_QSIM disabled! Building only processor models not using QSim.
    -----------------------------------------------------------------])
fi


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
