
# Build libdispatch, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/dispatch

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

lib_LTLIBRARIES = libbes_ppt.la

libbes_ppt_la_SOURCES = $(SRCS) $(HDRS)
libbes_ppt_la_CPPFLAGS = $(openssl_includes) $(kerberos_includes)
libbes_ppt_la_LDFLAGS = -version-info $(LIBPPT_VERSION)

pkginclude_HEADERS = $(HDRS)


SRCS:=PPTClient.cc Socket.cc TcpSocket.cc UnixSocket.cc SocketUtilities.cc \
	SocketListener.cc PPTServer.cc PPTProtocol.cc \
	PPTConnection.cc SSLClient.cc SSLServer.cc SSLConnection.cc

HDRS:=PPTClient.h Socket.h TcpSocket.h UnixSocket.h SocketUtilities.h \
	SocketException.h PPTException.h SocketConfig.h \
	SocketListener.h PPTServer.h PPTProtocol.h \
	PPTConnection.h Connection.h ServerHandler.h \
	SSLClient.h SSLServer.h SSLConnection.h

