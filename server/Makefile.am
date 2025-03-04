
# Build opendapcmdln, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS =  -I$(top_srcdir)/ppt  -I$(top_srcdir)/dispatch -I$(top_srcdir)/xmlcommand

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

EXTRA_DIST = beslog2json.py

bin_PROGRAMS = beslistener besdaemon
dist_bin_SCRIPTS = besctl hyraxctl

beslistener_SOURCES = BESServerHandler.cc ServerApp.cc BESServerUtils.cc \
BESServerHandler.h ServerApp.h BESServerUtils.h \
ServerExitConditions.h BESDaemonConstants.h

beslistener_CPPFLAGS = $(XML2_CFLAGS) $(AM_CPPFLAGS)
beslistener_LDADD = ../ppt/libbes_ppt.la ../xmlcommand/libbes_xml_command.la \
../dispatch/libbes_dispatch.la $(XML2_LIBS)

besdaemon_SOURCES = daemon.cc BESServerUtils.cc BESServerUtils.h \
DaemonCommandHandler.cc DaemonCommandHandler.h BESXMLWriter.cc \
BESXMLWriter.h BESDaemonConstants.h setgroups.c

besdaemon_CPPFLAGS = $(XML2_CFLAGS) $(AM_CPPFLAGS)
besdaemon_LDADD = ../ppt/libbes_ppt.la ../xmlcommand/libbes_xml_command.la \
../dispatch/libbes_dispatch.la  $(XML2_LIBS)

install-data-local:
	test -z "$(localstatedir)/run/bes" || $(MKDIR_P) "$(DESTDIR)$(localstatedir)/run/bes"

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	-mkdir $(C4_DIR)
	cccc --outdir=$(C4_DIR) $(beslistener_SOURCES) $(besdaemon_SOURCES)
