#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE([manifold_zesto], [1.0])
AC_CONFIG_SRCDIR([eval.h])
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.

# It appears Zesto must be compiled with -O0; the following 2 lines must
# appear before AC_PROG_CC/CXX to take effect.
: ${CXXFLAGS="-g -O0"}
: ${CFLAGS="-g -O0"}
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AM_PROG_CC_C_O  # required for C programs to use per-target flags

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lEI':
#AC_CHECK_LIB([EI], [main])
# FIXME: Replace `main' with a function in `-ldl':
#AC_CHECK_LIB([dl], [main])
# FIXME: Replace `main' with a function in `-lm':
#AC_CHECK_LIB([m], [main])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h float.h inttypes.h limits.h netdb.h stddef.h stdint.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_FSEEKO
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([floor getcwd memset pow rint socket sqrt strcasecmp strdup strrchr strtol strtoul])


# enable parallel simulation by default
AC_ARG_ENABLE([para_sim],
    [AS_HELP_STRING([--disable-para-sim],
        [disable parallel simulation @<:@default: no@:>@])],
    [para_sim=${enableval}], [para_sim=yes])

AC_SUBST(CC, g++)
if test "x${para_sim}" = xyes ; then
    AC_CHECK_PROGS([MPICPP], [mpic++], [mpic++])
    if test "${MPICPP}" = "mpic++" ; then # para_sim enabled; mpic++ found
        AC_SUBST(CXX, mpic++)
        AC_SUBST(CC, mpic++)
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
	AC_MSG_NOTICE([
    -----------------------------------------------
    Use QSim header files from $QSIMINC
    -----------------------------------------------
	])
    fi
else # use_qsim disabled
    AC_MSG_WARN([
    -----------------------------------------------------------------
    USE_QSIM disabled! Building only processor models not using QSim.
    -----------------------------------------------------------------])
fi



#The DEFAULT_INCLUDES is "-I.". However, Zesto has a file endian.h, which has the
#same name as a standard lib header file. For example, stdlib.h eventually includes
# endian.h. With "-I." set, stdlib.h will include Zesto's endian.h, resulting in
# compile errors. Therefore, here we set DEFAULT_INCLUDES to empty. Another solution
# is to rename endian.h.
AC_SUBST([DEFAULT_INCLUDES])


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
