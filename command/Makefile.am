
# Build libdispatch, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/dispatch

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

lib_LTLIBRARIES = libopendap_command.la
pkglib_LTLIBRARIES = libdap_cmdmodule.la

libopendap_command_la_SOURCES = $(SRCS) $(HDRS)
libopendap_command_la_LDFLAGS = -version-info $(LIBCOMMAND_VERSION)

libdap_cmdmodule_la_SOURCES = $(DAP_SRCS) $(DAP_HDRS)
libdap_cmdmodule_la_LDFLAGS = -version-info $(LIBCOMMAND_VERSION)

pkginclude_HEADERS = $(HDRS) $(DAP_HDRS)

##############################################################################
#
# Sources and Headers
#
##############################################################################
SRCS = BESCmdInterface.cc BESCmdParser.cc BESCommand.cc	     \
	BESGetCommand.cc BESSetCommand.cc \
	BESShowCommand.cc BESTermCommand.cc \
	BESParserException.cc BESTokenizer.cc \
	BESDeleteCommand.cc opendap_commands.cc

HDRS = BESCmdInterface.h BESCommand.h BESTokenizer.h \
	BESParserException.h opendap_commands.h BESCmdParser.h \
	BESGetCommand.h BESSetCommand.h \
	BESShowCommand.h BESDeleteCommand.h \
	BESTermCommand.h

DAP_SRCS = BESDefineCommand.cc BESSetContainerCommand.cc \
	BESCatalogCommand.cc DAPCommandModule.cc \
	BESDelContainerCommand.cc BESDelContainersCommand.cc \
	BESDelDefCommand.cc BESDelDefsCommand.cc

DAP_HDRS = BESDefineCommand.h BESSetContainerCommand.h \
	BESCatalogCommand.h DAPCommandModule.h \
	BESDelContainerCommand.h BESDelContainersCommand.h \
	BESDelDefCommand.h BESDelDefsCommand.h

