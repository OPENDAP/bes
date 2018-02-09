
# Build bes command library, part of the BES code.

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = -I$(top_srcdir)/dap -I$(top_srcdir)/dispatch $(XML2_CFLAGS)
AM_CXXFLAGS = 

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

SUBDIRS = . unit-tests

lib_LTLIBRARIES = libbes_xml_command.la
if LIBDAP
pkglib_LTLIBRARIES = libdap_xml_module.la
endif

libbes_xml_command_la_SOURCES = $(SRCS) $(HDRS)
libbes_xml_command_la_LDFLAGS = -version-info $(LIBXMLCOMMAND_VERSION)
libbes_xml_command_la_LIBADD = 

# ../dispatch/libbes_dispatch.la $(XML2_LIBS)

libdap_xml_module_la_SOURCES = $(DAP_SRCS) $(DAP_HDRS)
libdap_xml_module_la_LDFLAGS = -avoid-version -module
libdap_xml_module_la_LIBADD = 

#libbes_xml_command.la 

# ../dispatch/libbes_dispatch.la

pkginclude_HEADERS = $(HDRS) $(DAP_HDRS)

##############################################################################
#
# Sources and Headers
#
##############################################################################
SRCS = BESXMLInterface.cc BESXMLCommand.cc BESXMLUtils.cc		\
	BESXMLDefaultCommands.cc BESXMLShowCommand.cc			\
	BESXMLShowErrorCommand.cc					\
	BESXMLSetContextCommand.cc BESXMLSetContainerCommand.cc		\
	BESXMLDefineCommand.cc BESXMLGetCommand.cc			\
	BESXMLDeleteContainerCommand.cc BESXMLDeleteContainersCommand.cc\
	BESXMLDeleteDefinitionCommand.cc BESXMLDeleteDefinitionsCommand.cc \
	SetContextsResponseHandler.cc XMLSetContextsCommand.cc

HDRS = BESXMLInterface.h BESXMLCommand.h BESXMLUtils.h			\
	BESXMLDefaultCommands.h BESXMLShowCommand.h			\
	BESXMLShowErrorCommand.h					\
	BESXMLSetContextCommand.h BESXMLSetContainerCommand.h		\
	BESXMLDefineCommand.h BESXMLGetCommand.h			\
	BESXMLDeleteContainerCommand.h BESXMLDeleteContainersCommand.h	\
	BESXMLDeleteDefinitionCommand.h BESXMLDeleteDefinitionsCommand.h \
	SetContextsResponseHandler.h XMLSetContextsCommand.h NULLResponseHandler.h
	
DAP_SRCS = BESXMLDapCommandModule.cc BESXMLCatalogCommand.cc
DAP_HDRS = BESXMLDapCommandModule.h BESXMLCatalogCommand.h

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	-mkdir $(C4_DIR)
	cccc --outdir=$(C4_DIR) $(SRCS) $(DAP_SRCS) $(HDRS) $(DAP_HDRS)
