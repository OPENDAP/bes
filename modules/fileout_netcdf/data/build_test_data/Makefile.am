# Tests

if DAP_MODULES
AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap -I$(top_srcdir)/modules/fileout_netcdf \
$(NC_CPPFLAGS) $(DAP_CFLAGS)
# Forcing the link with the static archive removes a warning about linking
# with 'modules' not being portable. jhrg 11/30/15
LIBADD = $(NC_LDFLAGS) $(NC_LIBS) -L$(top_builddir)/dap -ldap_module $(BES_DISPATCH_LIB) \
$(BES_EXTRA_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS)
# $(top_builddir)/dap/.libs/libdap_module.a
else
AM_CPPFLAGS = -I$(top_srcdir) $(BES_CPPFLAGS) # or wahtever was set here or in ..._CPPFLAGS
LIBADD = $(BES_DAP_LIBS)      # and ..._LIBADD
endif

# AM_CPPFLAGS =  $(BES_CPPFLAGS)
# There actually is no AM_LDADD in automake, so we should probably get
# rid of this and use a name that's not so confusing - makes it seem
# like automake does something to manage this, but it doesn't
AM_LDADD = $(LIBADD)
AM_LDFLAGS = -static

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror

DISTCLEANFILES = 

CLEANFILES = *.dods *.das

EXTRA_DIST = 

# These programs are used to build the .dods files used by the tests in
# the 'tests' directory
noinst_PROGRAMS = simpleT00 structT00 structT01 structT02 arrayT arrayT01 \
gridT seqT attrT namesT

############################################################################

# readT

SRCS = test_send_data.cc test_send_data.h

simpleT00_SOURCES = simpleT00.cc $(SRCS)
simpleT00_LDADD = $(AM_LDADD)

arrayT_SOURCES = arrayT.cc $(SRCS)
arrayT_LDADD = $(AM_LDADD)

arrayT01_SOURCES = arrayT01.cc $(SRCS)
arrayT01_LDADD = $(AM_LDADD)

structT00_SOURCES = structT00.cc $(SRCS)
structT00_LDADD = $(AM_LDADD)

structT01_SOURCES = structT01.cc $(SRCS)
structT01_LDADD = $(AM_LDADD)

structT02_SOURCES = structT02.cc $(SRCS)
structT02_LDADD = $(AM_LDADD)

attrT_SOURCES = attrT.cc $(SRCS)
attrT_LDADD = $(AM_LDADD)

gridT_SOURCES = gridT.cc $(SRCS)
gridT_LDADD = $(AM_LDADD)

namesT_SOURCES = namesT.cc $(SRCS)
namesT_LDADD = $(OBJS) $(AM_LDADD)

seqT_SOURCES = seqT.cc $(SRCS)
seqT_LDADD = $(AM_LDADD)

# This was/is a way to read data from .dods files and build netcdf
# that worked without using the dapreader handler (before that was
# working). Keep it around if it's useful. jhrg 11/29/15 
#
# readT_SOURCES = readT.cc $(SRCS) ReadTypeFactory.cc ReadTypeFactory.h ReadSequence.cc ReadSequence.h
# readT_LDADD = $(OBJS) $(AM_LDADD) $(DAP_CLIENT_LIBS)

