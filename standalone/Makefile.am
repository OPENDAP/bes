
# Build bescmdln, part of the BES code.

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = -I$(top_srcdir)/ppt -I$(top_srcdir)/xmlcommand -I$(top_srcdir)/cmdln -I$(top_srcdir)/dispatch $(XML2_CFLAGS)
AM_CXXFLAGS =

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

lib_LTLIBRARIES = libbes_standalone.la
libbes_standalone_la_SOURCES = $(besstandalone_SOURCES)
libbes_standalone_la_LDFLAGS = -version-info $(LIBDISPATCH_VERSION)
libbes_standalone_la_LIBADD = $(DAP_LIBS) $(XML2_LIBS) $(BES_ZLIB_LIBS) \
    $(BES_BZ2_LIBS) $(BES_DL_LIBS) $(PTHREAD_LIBS)


# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

bin_PROGRAMS = besstandalone

besstandalone_SOURCES = besstandalone.cc StandAloneApp.cc StandAloneClient.cc \
StandAloneApp.h StandAloneClient.h

# 			../cmdln/CmdTranslation.cc \
#			../cmdln/CmdTranslation.h \
# 			../xmlcommand/BESXMLUtils.cc \
#			../xmlcommand/BESXMLUtils.h

besstandalone_CPPFLAGS = $(XML2_CFLAGS) $(AM_CPPFLAGS)

# Added libbes_dap.la jhrg 12/24/12 so besstandalone can call BESCatalogUtils::delete_all_catalogs()

besstandalone_LDADD = $(top_builddir)/dispatch/libbes_dispatch.la  \
$(top_builddir)/xmlcommand/libbes_xml_command.la $(top_builddir)/cmdln/CmdTranslation.o \
$(READLINE_LIBS) $(XML2_LIBS)
