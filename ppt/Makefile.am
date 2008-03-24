
# Build libbes_ppt, part of the BES code.

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

SUBDIRS = unit-tests testsuite
DIST_SUBDIRS = unit-tests testsuite

lib_LTLIBRARIES = libbes_pptcapi.la libbes_ppt.la

libbes_ppt_la_SOURCES = $(SRCS) $(HDRS)
libbes_ppt_la_CPPFLAGS = $(AM_CPPFLAGS)
libbes_ppt_la_LDFLAGS = -version-info $(LIBPPTCAPI_VERSION)
libbes_ppt_la_LIBADD = ../dispatch/libbes_dispatch.la

libbes_pptcapi_la_SOURCES = $(CSRCS) $(CHDRS)
libbes_pptcapi_la_LDFLAGS = -version-info $(LIBPPT_VERSION)

pkginclude_HEADERS = $(HDRS)


SRCS = PPTClient.cc Socket.cc TcpSocket.cc UnixSocket.cc SocketUtilities.cc \
	SocketListener.cc PPTServer.cc PPTProtocol.cc			    \
	PPTConnection.cc Connection.cc PPTStreamBuf.cc

HDRS = PPTClient.h Socket.h TcpSocket.h UnixSocket.h SocketUtilities.h	\
	SocketConfig.h SocketListener.h PPTServer.h PPTProtocol.h	\
	PPTConnection.h Connection.h ServerHandler.h PPTStreamBuf.h

CSRCS = pptcapi_tcp_connect.c pptcapi_socket_connect.c pptcapi.c

CHDRS = pptcapi.h

if HAVE_OPENSSL
libbes_ppt_la_CPPFLAGS += $(openssl_includes)
libbes_ppt_la_LIBADD += $(openssl_libs)
SRCS+=SSLClient.cc SSLServer.cc SSLConnection.cc
HDRS+=SSLClient.h SSLServer.h SSLConnection.h
endif
