# prelude
AC_INIT([EXTOLL2 RMA2 API],[2.1],[mondrian.nuessle@extoll.de])
AC_CONFIG_AUX_DIR([admin])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PREFIX_DEFAULT([/extoll2])
AC_CONFIG_MACRO_DIR([admin])

# fix for  new versions of autotools
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])


# checks for programs
AC_PROG_CC
#AC_PROG_GCC_TRADITIONAL
LT_INIT
AC_PROG_INSTALL

# checks for libs
EXTOLL2_CHECK_DRV

# checks for headers
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([sys/ioctl.h])

# checks for typedefs, stuctures and compiler characteristics
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# checks for lib functions
AC_CHECK_FUNCS([getpagesize])
AC_FUNC_MALLOC
AC_FUNC_MMAP

# output files
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile 
		 include/Makefile 
		 src/Makefile
		 tests/Makefile
		 examples/Makefile
		 doc/Makefile])
		 #tests/Makefile
AC_OUTPUT
