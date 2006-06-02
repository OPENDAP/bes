
# Build libdispatch, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/dispatch

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

lib_LTLIBRARIES = libopendap_command.la libdap_cmdmodule.la

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
SRCS:=OPeNDAPCmdInterface.cc OPeNDAPCmdParser.cc OPeNDAPCommand.cc	     \
	OPeNDAPGetCommand.cc OPeNDAPSetCommand.cc \
	OPeNDAPShowCommand.cc OPeNDAPTermCommand.cc \
	OPeNDAPParserException.cc OPeNDAPTokenizer.cc \
	OPeNDAPDeleteCommand.cc opendap_commands.cc

HDRS:=OPeNDAPCmdInterface.h OPeNDAPCommand.h OPeNDAPTokenizer.h \
	OPeNDAPParserException.h opendap_commands.h

DAP_SRCS:=OPeNDAPDefineCommand.cc OPeNDAPSetContainerCommand.cc \
	OPeNDAPCatalogCommand.cc DAPCommandModule.cc \
	OPeNDAPDelContainerCommand.cc OPeNDAPDelContainersCommand.cc \
	OPeNDAPDelDefCommand.cc OPeNDAPDelDefsCommand.cc

DAP_HDRS:=OPeNDAPDefineCommand.h OPeNDAPSetContainerCommand.h \
	OPeNDAPCatalogCommand.h DAPCommandModule.h \
	OPeNDAPDelContainerCommand.h OPeNDAPDelContainersCommand.h \
	OPeNDAPDelDefCommand.h OPeNDAPDelDefsCommand.h

