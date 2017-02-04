
# Build libdap, libdap_test-types.a, getdap and deflate

AUTOMAKE_OPTIONS = foreign 

# Arrange to build with the backward compatibility mode enabled.
AM_CPPFLAGS = -DFREEFORM $(FF_CPPFLAGS)

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

noinst_LTLIBRARIES = libfreeform.la

noinst_PROGRAMS = readfile newform chkform bufform checkvar

libfreeform_la_SOURCES = $(FF_SRCS) $(FF_HDRS)

readfile_SOURCES = readfile.c
# Comment these *_CPPFLAGS lines if AM_CPPFLAGS can be used instead.
# readfile_CPPFLAGS = -DFREEFORM
readfile_LDADD = libfreeform.la -lm

newform_SOURCES = newform.c
# newform_CPPFLAGS = -DFREEFORM
newform_LDADD = libfreeform.la -lm

chkform_SOURCES = chkform.c
chkform_CPPFLAGS = -DFF_MAIN -DFREEFORM $(FF_CPPFLAGS)
chkform_LDADD = libfreeform.la -lm

bufform_SORUCES = bufform.c
bufform_CPPFLAGS = -DFF_MAIN -DFREEFORM $(FF_CPPFLAGS)
bufform_LDADD = libfreeform.la -lm

checkvar_SOURCES = checkvar.c
# checkvar_CPPFLAGS = -DFREEFORM
checkvar_LDADD = libfreeform.la -lm

EXTRA_DIST = 

CLEANFILES = *~

FF_SRCS = afm2bfm.c avlins.c avlfree.c cv_units.c dbevents.c dl_lists.c \
	  eqn_util.c error.c eval_eqn.c ff_utils.c file2buf.c formlist.c \
	  freeform.c makedbin.c makeform.c name_tab.c ndarray.c os_utils.c \
	  proclist.c setdbin.c showdbin.c mm_make.c

FF_HDRS = avl.h err.h ff_types.h memtrack.h ndarray.h		\
	avltree.h eval_eqn.h freeform.h menuindx.h os_utils.h	\
	dl_lists.h ff2psdts.h maxmin.h name_tab.h

###########################################################################

# Fortify targets.

.PHONY: fortify
fortify:
	sourceanalyzer -b @PACKAGE@ $(MAKE) libfreeform.la
	sourceanalyzer -b @PACKAGE@ -scan -f ../@PACKAGE@-@PACKAGE_VERSION@.fpr

# Use this to clean the fortify project.
.PHONY: fortifyclean
fortifyclean:
	sourceanalyzer -b @PACKAGE@ -clean
