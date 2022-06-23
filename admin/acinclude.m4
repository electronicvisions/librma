dnl ------------------------------------------------------------------------
dnl Find a file (or one of more files in a list of dirs)
dnl  - RETURNS THE FOLDER WHERE THE FILE WAS FOUND!
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([EXTOLL_FIND_FILE],
[
$3=NO
for i in $2;
do
  for j in $1;
  do
    echo "configure: __oline__: $i/$j" >&AC_FD_CC
    if test -r "$i/$j"; then
      echo "taking that" >&AC_FD_CC
      $3=$i
      break 2
    fi
  done
done
])



dnl ------------------------------------------------------------------------
dnl check for EXTOLLDRV
dnl - change by rleys: Add to -I the folder where the header was found...then just add to EXTOLL_FIND_FILE a possible path, and it will always work
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN([EXTOLL2_CHECK_DRV],
[ AC_MSG_CHECKING(for EXTOLL2 driver)
  EXTOLL_FIND_FILE([rma2_region.h], [$EXTOLL_R2_HOME/include/ ${EXTOLL_SW}/extoll_r2/extoll2_drv/ ${PREFIX}/include], drv_h)
 if test $drv_h = NO; then
  AC_MSG_ERROR([cannot find EXTOLL2 driver])
 else
  drv="$drv_h"
 fi  
 EXTOLL2DRV_INCLUDES="-I$drv"
 AC_SUBST(EXTOLL2DRV_INCLUDES)
 AC_MSG_RESULT([includes $EXTOLL2DRV_INCLUDES])
])

