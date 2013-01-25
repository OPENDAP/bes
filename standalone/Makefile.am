
# Build bescmdln, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/dap -I$(top_srcdir)/ppt -I$(top_srcdir)/xmlcommand -I$(top_srcdir)/cmdln -I$(top_srcdir)/dispatch

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

bin_PROGRAMS = besstandalone

besstandalone_SOURCES = StandAloneApp.cc StandAloneClient.cc \
			../cmdln/CmdTranslation.cc \
			../xmlcommand/BESXMLUtils.cc \
			StandAloneApp.h StandAloneClient.h \
			../cmdln/CmdTranslation.h \
			../xmlcommand/BESXMLUtils.h

besstandalone_CPPFLAGS = $(XML2_CFLAGS) $(AM_CPPFLAGS)
# Added libbes_dap.la jhrg 12/24/12 so besstandalone can call BESCatalogUtils::delete_all_catalogs()
besstandalone_LDADD = ../dispatch/libbes_dispatch.la ../dispatch/libbes_dap.la \
	../xmlcommand/libbes_xml_command.la $(READLINE_LIBS) $(XML2_LIBS)
