

prefix = $(DODS_ROOT)
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
libdir = $(exec_prefix)/lib
includedir = $(prefix)/include
AR = ar
RANLIB = ranlib
INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_PROGRAM = ${INSTALL}

PROGS:=dodsserverd xdods
XDODSSRCS:=ServerMain.cc Server.cc
XDODSOBJS:=ServerMain.o Server.o
XDODSLIBS:=../cedar-handler/cedar_module.o ../cedar-handler/cedar_persistence.o ../cedar-handler/libcedar-handler.a ../dispatch/test_authenticator.o ../dispatch/dods_module.o ../dispatch/no_aggserver.o ../ppt/libdodsppt.a  ../dispatch/libdispatch.a ../dap/libdap++.a
XDODSLDFLAGS:=$(XDODSLIBS) -L/project/cedar/lib/Cedar-2.0.1/lib -lCedar -L/project/cedar/lib/dap-3.4.6/lib -lxml2 -lz -lpthread -lm -lrx -L/project/cedar/lib/dap-3.4.6/lib -lcurl -L/usr/kerberos/lib -lz -lssl -lcrypto -ldl -lssl -lcrypto -lgssapi_krb5 -lkrb5 -lcom_err -lk5crypto -lresolv -ldl -lz -lz -lpthread

LDFLAGS= -L../../lib 

DAEMONSRCS:=ServerDaemon.cc
DAEMONOBJS:=ServerDaemon.o
DAEMONLIBS:=
HDRS:=

INCS:=-I. -I../ppt -I../dispatch -I../dap

DEFS:=-DHAVE_CONFIG_H

CXXFLAGS_DEBUG:=-g3 -O0 -Wall -fno-default-inline -fno-defer-pop
CXXFLAGS:=$(CXXFLAGS_DEBUG) 
# CXXFLAGS:=-gstabs -O2  -Wall -Wno-deprecated -DNDEBUG
CPPFLAGS:=$(CXXFLAGS) $(DEFS) $(INCS)

all : $(PROGS)

xdods : $(XDODSOBJS) $(XDODSLIBS)
	g++ -o xdods $(XDODSOBJS) $(XDODSLDFLAGS)

dodsserverd : $(DAEMONOBJS)
	g++ -o dodsserverd $(DAEMONOBJS) $(DAEMONLIBS)

%.o : %.cc
	g++ -c $(CPPFLAGS) $<

install: all install-headers install-mods
	-mkdir -p $(libdir)
	$(INSTALL_DATA) $(LIB) $(libdir)
	$(RANLIB) $(libdir)/$(LIB)

install-headers:
	-mkdir -p $(includedir)
	for h in $(HDRS); \
	    do $(INSTALL_DATA) $$h $(includedir); done

install-mods:
	-mkdir -p $(libdir)
	for h in $(MODOBJS); \
	    do $(INSTALL_DATA) $$h $(libdir); done

Makefile: Makefile.in
	if [ -x ./config.status ]; \
	then \
	    ${SHELL} ./config.status; \
	else \
	    ./configure; \
	fi

.PHONY: depend
depend: 
	../..//etc/depend.sh -m Makefile.in -- $(CPPFLAGS) -- $(XDODSSRCS) $(DAEMONSRCS)
	$(MAKE) $(MFLAGS) Makefile

clean :
	/bin/rm -rf $(XDODSOBJS) $(DAEMONOBJS)

# DO NOT DELETE; depend requires this line
ServerMain.o: ServerMain.cc Server.h ../dispatch/DODSGlobalIQ.h \
  ../dispatch/DODSException.h
Server.o: Server.cc Server.h ../ppt/PPTSocket.h ../ppt/PPTException.h \
  ../ppt/PPTBuffer.h ../ppt/PPTUtilities.h ../ppt/PPTServerListener.h \
  ../ppt/PPTClientServerSessionProtocol.h ServerStatusControl.h \
  ServerExitConditions.h ../dispatch/DODSDataRequestInterface.h \
  ../dispatch/DODSWrapper.h ../dispatch/DODSMemoryGlobalArea.h \
  ../dispatch/TheDODSKeys.h ../dispatch/DODSKeys.h \
  ../dispatch/DODSKeysException.h ../dispatch/DODSBasicException.h \
  ../dispatch/DODSException.h ../dispatch/DODSStatusReturn.h
ServerDaemon.o: ServerDaemon.cc ServerExitConditions.h
