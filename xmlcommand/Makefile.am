
# Build bes command library, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/dispatch $(XML2_CFLAGS)

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

SUBDIRS = unit-tests
DIST_SUBDIRS = unit-tests

lib_LTLIBRARIES = libbes_xml_command.la
if LIBDAP
pkglib_LTLIBRARIES = libdap_xml_module.la
endif

libbes_xml_command_la_SOURCES = $(SRCS) $(HDRS)
libbes_xml_command_la_LDFLAGS = -version-info $(LIBXMLCOMMAND_VERSION)
libbes_xml_command_la_LIBADD = ../dispatch/libbes_dispatch.la

libdap_xml_module_la_SOURCES = $(DAP_SRCS) $(DAP_HDRS)
libdap_xml_module_la_LDFLAGS = -avoid-version -module
libdap_xml_module_la_LIBADD = libbes_xml_command.la ../dispatch/libbes_dispatch.la

pkginclude_HEADERS = $(HDRS) $(DAP_HDRS)

##############################################################################
#
# Sources and Headers
#
##############################################################################
SRCS = BESXMLInterface.cc BESXMLCommand.cc BESXMLUtils.cc		\
	BESXMLDefaultCommands.cc BESXMLShowCommand.cc			\
	BESXMLSetContextCommand.cc BESXMLSetContainerCommand.cc		\
	BESXMLDefineCommand.cc BESXMLGetCommand.cc			\
	BESXMLDeleteContainerCommand.cc BESXMLDeleteContainersCommand.cc\
	BESXMLDeleteDefinitionCommand.cc BESXMLDeleteDefinitionsCommand.cc

HDRS = BESXMLInterface.h BESXMLCommand.h BESXMLUtils.h			\
	BESXMLDefaultCommands.h BESXMLShowCommand.h			\
	BESXMLSetContextCommand.h BESXMLSetContainerCommand.h		\
	BESXMLDefineCommand.h BESXMLGetCommand.h			\
	BESXMLDeleteContainerCommand.h BESXMLDeleteContainersCommand.h	\
	BESXMLDeleteDefinitionCommand.h BESXMLDeleteDefinitionsCommand.h

DAP_SRCS = BESXMLDapCommandModule.cc BESXMLGetDataDDXCommand.cc		\
	BESXMLCatalogCommand.cc
DAP_HDRS = BESXMLDapCommandModule.h BESXMLGetDataDDXCommand.h		\
	BESXMLCatalogCommand.h
