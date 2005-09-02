
# Build libdispatch, part of the BES code.

AUTOMAKE_OPTIONS = foreign

# AM_CPPFLAGS =
# AM_CXXFLAGS =

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

lib_LTLIBRARIES = libdispatch.la libmodules.la

libopendap_command_la_SOURCES = $(SRCS) $(HDRS)

libopendap_command_la_LDFLAGS = -version-info $(LIBDISPATCH_VERSION)

pkginclude_HEADERS = $(HDRS)

# noinst_HEADERS = 

# EXTRA_DIST = 

# CLEANFILES = 

##############################################################################
#
# Sources and Headers for 
SRCS:=OPeNDAPCmdInterface.cc OPeNDAPCmdParser.cc OPeNDAPCommand.cc \
	OPeNDAPDefineCommand.cc OPeNDAPDeleteCommand.cc OPeNDAPGetCommand.cc \
	OPeNDAPSetCommand.cc OPeNDAPShowCommand.cc OPeNDAPTermCommand.cc

HDRS:=OPeNDAPCmdInterface.h OPeNDAPCmdParser.h OPeNDAPCommand.h

MOD_SRCS:=opendap_commands.cc
MOD_OBJS:=opendap_commands.o

TEST_SRC:=cmd_test.cc
TEST_OBJ:=cmd_test.o
TEST_EXEC:=cmd_test

INCS:=-I. -I../dispatch
LIB:=libopendap_command.a
DEFS:=@DEFS@
#CXXFLAGS_DEBUG:=-g3 -O0 -Wall -fno-default-inline -fno-defer-pop
#CXXFLAGS:=$(CXXFLAGS_DEBUG) 
CXXFLAGS:=@CXXFLAGS@ -Wall -Wno-deprecated
CPPFLAGS:=$(CXXFLAGS) $(DEFS) $(INCS)

LIBS:=../dispatch/dods_module.o -L../dispatch -ldispatch \
	-L/project/cedar/src/pwest/opendap-g/lib -ldap  -L/project/cedar/src/pwest/packages/lib -lcurl -L/usr/kerberos/lib -lssl -lcrypto -ldl -lssl -lcrypto -lgssapi_krb5 -lkrb5 -lcom_err -lk5crypto -lresolv -ldl -lz -lz -L/usr/lib -lxml2 -lz -lpthread -lm -lpthread 

all : $(LIB) $(MOD_OBJS)

%.o : %.cc
	$(CXX) -c $(CPPFLAGS) $<

$(LIB) : $(OBJS)
	$(AR) cru $@ $?
	$(RANLIB) $@

test : $(TEST_EXEC)

$(TEST_EXEC) : $(TEST_OBJ) $(LIB) $(MOD_OBJS)
	$(CXX) -o $(TEST_EXEC) $(TEST_OBJ) $(MOD_OBJS) $(LIB) $(LIBS)

