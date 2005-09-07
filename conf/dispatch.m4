# check for dap libraries, not in usual location
#
# Use AC_CHECK_LIBDAP instead. jhrg 9/2/05

AC_DEFUN([DISPATCH_DAP], [dnl
    AC_ARG_WITH(dap,
        [  --with-dap=ARG       Where is the external dap sources (directory)],
        DAP_PATH=${withval}, DAP_PATH="${OPENDAP_ROOT}")

        LIBS="`$DAP_PATH/bin/dap-config --libs` $LIBS"
	INCS="`$DAP_PATH/bin/dap-config --cflags` $INCS"
	CXXFLAGS="$CXXFLAGS -DDEFAULT_BASETYPE_FACTORY"

	AC_SUBST(INCS)
	AC_SUBST(LIBS)
	AC_SUBST(CXXFLAGS)
    ])

# check for mysql libraries
AC_DEFUN([DISPATCH_MYSQL], [dnl
    AC_ARG_WITH(mysql,
        [  --with-mysql=ARG          Where is the MySQL library (directory)],
        MYSQL_PATH=${withval}, MYSQL_PATH="not_used")
    AC_MSG_CHECKING([for MySQL])
    if test -d "$MYSQL_PATH"
    then
	    AC_MSG_RESULT(using MySQL in $MYSQL_PATH)
	    DISPATCHSRCS="\$(SRCS) \$(MYSQLSRCS)"
            AC_SUBST(DISPATCHSRCS)
	    DISPATCHOBJS="\$(OBJS) \$(MYSQLOBJS)"
            AC_SUBST(DISPATCHOBJS)
	    DISPATCHHDRS="\$(HDRS) \$(MYSQLHDRS)"
            AC_SUBST(DISPATCHHDRS)
            LIBS="$LIBS -L${MYSQL_PATH}/lib/mysql -lmysqlclient"
            AC_SUBST(LIBS)
            INCS="$INCS -I${MYSQL_PATH}/include/mysql"
            AC_SUBST(INCS)
    else
	    AC_MSG_RESULT(NOT using MySQL)
	    DISPATCHSRCS="\$(SRCS)"
            AC_SUBST(DISPATCHSRCS)
	    DISPATCHOBJS="\$(OBJS)"
            AC_SUBST(DISPATCHOBJS)
	    DISPATCHHDRS="\$(HDRS)"
            AC_SUBST(DISPATCHHDRS)
    fi])

# check for dispatch version
# 
# Get teh version number from configure.ac instead. jhrg 9/2/05

AC_DEFUN([DISPATCH_DIRECTORY_VERSION], [dnl
    AC_MSG_CHECKING([version])
    VERSION=`cat version.h`
    AC_MSG_RESULT($VERSION)
    AC_SUBST(VERSION)])

