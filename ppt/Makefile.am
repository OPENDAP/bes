
# Build libbes_ppt, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/dispatch -I$(top_srcdir)/server

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

SUBDIRS = . unit-tests
# DIST_SUBDIRS = unit-tests

lib_LTLIBRARIES = libbes_ppt.la

libbes_ppt_la_SOURCES = $(SRCS) $(HDRS)
libbes_ppt_la_CPPFLAGS = $(AM_CPPFLAGS)
libbes_ppt_la_LDFLAGS = -version-info $(LIBPPT_VERSION)
libbes_ppt_la_LIBADD = ../dispatch/libbes_dispatch.la

pkginclude_HEADERS = $(HDRS)

SRCS = PPTClient.cc Socket.cc TcpSocket.cc UnixSocket.cc SocketUtilities.cc \
	SocketListener.cc PPTServer.cc \
	PPTConnection.cc Connection.cc PPTStreamBuf.cc

HDRS = PPTClient.h Socket.h TcpSocket.h UnixSocket.h SocketUtilities.h	\
	SocketConfig.h SocketListener.h PPTServer.h PPTProtocolNames.h	\
	PPTConnection.h Connection.h ServerHandler.h PPTStreamBuf.h

#if HAVE_OPENSSL
#libbes_ppt_la_CPPFLAGS += $(openssl_includes)
#libbes_ppt_la_LIBADD += $(openssl_libs)
#SRCS+=SSLClient.cc SSLServer.cc SSLConnection.cc
#HDRS+=SSLClient.h SSLServer.h SSLConnection.h
#endif

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	-mkdir $(C4_DIR)
	cccc --outdir=$(C4_DIR) $(SRCS) $(HDRS)
