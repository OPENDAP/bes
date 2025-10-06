# Tests

if DAP_MODULES

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/modules/fileout_netcdf \
$(NC_CPPFLAGS) $(DAP_CFLAGS)
LIBADD = $(NC_LDFLAGS) $(NC_LIBS) $(BES_DISPATCH_LIB) -L$(top_builddir)/dap -ldap_module \
$(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)

# jhrg 6/2/23 $(BES_EXTRA_LIBS)

else

AM_CPPFLAGS = -I$(top_srcdir) $(BES_CPPFLAGS) # or whatever was set here or in ..._CPPFLAGS
LIBADD = $(BES_DAP_LIBS)      # and ..._LIBADD

endif

# AM_CPPFLAGS =  $(BES_CPPFLAGS)
# There actually is no AM_LDADD in automake, so we should probably get
# rid of this and use a name that's not so confusing - makes it seem
# like automake does something to manage this, but it doesn't
AM_LDADD = $(LIBADD)

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

DISTCLEANFILES = test_config.h atconfig

# CLEANFILES = 

EXTRA_DIST = data baselines test_config.h.in bes.conf.in $(TESTSUITE).at \
$(TESTSUITE) atlocal.in $(srcdir)/package.m4

# The check_PROGRAMS will be run before the autotest tests below (which
# are run using check-local). Note that the check_PROGRAMS are not really
# tests; they build various netcdf files that we then test using ncdump.
# jhrg 3/25/15
# check_PROGRAMS = $(DRIVERS)
noinst_PROGRAMS = $(DRIVERS)

############################################################################
# Unit Tests
#

DRIVERS = simpleT00 simpleT01 simpleT02 structT00 arrayT structT01	\
	structT02 arrayT01 gridT seqT attrT namesT readT

SRCS = test_send_data.cc test_send_data.h

# I replaced the SRCS variable and its references to ../<file>.cc
# with this to work-around a problem with distclean. jhrg 3/25/15
OBJS = ../FONcTransform.o ../FONcUtils.o ../FONcByte.o ../FONcStr.o	\
	../FONcShort.o ../FONcInt.o ../FONcFloat.o ../FONcDouble.o	\
	../FONcStructure.o ../FONcGrid.o ../FONcArray.o			\
	../FONcSequence.o ../FONcBaseType.o ../FONcDim.o ../FONcMap.o	\
	../FONcAttributes.o ../FONcRequestHandler.o

simpleT00_SOURCES = simpleT00.cc $(SRCS)
simpleT00_LDADD = $(OBJS) $(AM_LDADD)

simpleT01_SOURCES = simpleT01.cc $(SRCS)
simpleT01_LDADD = $(OBJS) $(AM_LDADD)

simpleT02_SOURCES = simpleT02.cc $(SRCS)
simpleT02_LDADD = $(OBJS) $(AM_LDADD)

arrayT_SOURCES = arrayT.cc $(SRCS)
arrayT_CPPFLAGS = $(AM_CPPFLAGS)
arrayT_LDADD = $(OBJS) $(AM_LDADD)

arrayT01_SOURCES = arrayT01.cc $(SRCS)
arrayT01_LDADD = $(OBJS) $(AM_LDADD)

structT00_SOURCES = structT00.cc $(SRCS)
structT00_LDADD = $(OBJS) $(AM_LDADD)

structT01_SOURCES = structT01.cc $(SRCS)
structT01_LDADD = $(OBJS) $(AM_LDADD)

structT02_SOURCES = structT02.cc $(SRCS)
structT02_LDADD = $(OBJS) $(AM_LDADD)

attrT_SOURCES = attrT.cc $(SRCS)
attrT_LDADD = $(OBJS) $(AM_LDADD)

gridT_SOURCES = gridT.cc $(SRCS)
gridT_LDADD = $(OBJS) $(AM_LDADD)

namesT_SOURCES = namesT.cc $(SRCS)
namesT_LDADD = $(OBJS) $(AM_LDADD)

seqT_SOURCES = seqT.cc $(SRCS)
seqT_LDADD = $(OBJS) $(AM_LDADD)

readT_SOURCES = readT.cc $(SRCS) ReadTypeFactory.cc ReadTypeFactory.h ReadSequence.cc ReadSequence.h
readT_LDADD = $(OBJS) $(AM_LDADD) $(DAP_CLIENT_LIBS)

############## Autotest follows #####################

# check-local: $(UNIT_TESTS) atconfig atlocal $(TESTSUITE)
# 	export PATH="@NC_BIN_PATH@:$$PATH"; $(TESTSUITE) $(TESTSUITEFLAGS)

AUTOM4TE = autom4te

TESTSUITE = $(srcdir)/fonc_handlerTest
TESTSUITEFLAGS =

$(TESTSUITE): $(TESTSUITE).at $(srcdir)/package.m4

check-local: atconfig atlocal $(srcdir)/package.m4 $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS) PATH="@NC_BIN_PATH@:$$PATH"

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	-rm -f $(TESTSUITE) $(srcdir)/package.m4 

AUTOTEST = $(AUTOM4TE) --language=autotest

$(TESTSUITE): $(TESTSUITE).at
	$(AUTOTEST) -I '$(srcdir)' -o $@ $@.at

# The `:;' works around a Bash 3.2 bug when the output is not writable.
$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	:;{ \
	echo '# Signature of the current package.' && \
	echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])' && \
	echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])' && \
	echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])' && \
	echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])' && \
	echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	} >'$(srcdir)/package.m4'

