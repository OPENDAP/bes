
# Build bes command library, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/dispatch

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

lib_LTLIBRARIES = libbes_command.la
pkglib_LTLIBRARIES = libdap_cmd_module.la

libbes_command_la_SOURCES = $(SRCS) $(HDRS)
libbes_command_la_LDFLAGS = -version-info $(LIBCOMMAND_VERSION)
libbes_command_la_LIBADD = ../dispatch/libbes_dispatch.la

libdap_cmd_module_la_SOURCES = $(DAP_SRCS) $(DAP_HDRS)
libdap_cmd_module_la_LDFLAGS = -avoid-version -module
libdap_cmd_module_la_LIBADD = libbes_command.la ../dispatch/libbes_dispatch.la

pkginclude_HEADERS = $(HDRS) $(DAP_HDRS)

##############################################################################
#
# Sources and Headers
#
##############################################################################
SRCS = BESCmdInterface.cc BESCmdParser.cc BESCommand.cc		\
	BESGetCommand.cc BESSetCommand.cc 			\
	BESShowCommand.cc BESTermCommand.cc 			\
	BESDefineCommand.cc BESSetContainerCommand.cc 		\
	BESDelContainerCommand.cc BESDelContainersCommand.cc 	\
	BESDelDefCommand.cc BESDelDefsCommand.cc		\
	BESTokenizer.cc 					\
	BESDeleteCommand.cc BESDefaultCommands.cc		\
	BESSetContextCommand.cc

HDRS = BESCmdInterface.h BESCommand.h BESTokenizer.h 		\
	BESParserException.h BESDefaultCommands.h BESCmdParser.h 	\
	BESGetCommand.h BESSetCommand.h 			\
	BESShowCommand.h BESDeleteCommand.h 			\
	BESDelContainerCommand.h BESDelContainersCommand.h 	\
	BESDelDefCommand.h BESDelDefsCommand.h			\
	BESDefineCommand.h BESSetContainerCommand.h 		\
	BESSetContextCommand.h BESTermCommand.h

DAP_SRCS = BESCatalogCommand.cc DAPCommandModule.cc
DAP_HDRS = BESCatalogCommand.h DAPCommandModule.h
