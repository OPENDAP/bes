
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

lib_LTLIBRARIES = libdispatch.la

libdispatch_la_SOURCES = $(SRCS) $(HDRS)

libdispatch_la_LDFLAGS = -version-info $(LIBDISPATCH_VERSION)

pkginclude_HEADERS = $(HDRS)

all-local: $(MODOBJS)

install-data-local: $(MODOBJS)
	test -z "$(pkglibdir)" || $(mkdir_p) "$(DESTDIR)$(pkglibdir)"
	list='$(MODOBJS)'; for p in $$list; do \
	  if test -f $$p; then \
	    f=$(am__strip_dir) \
	    $(INSTALL) $$p $(DESTDIR)$(pkglibdir)/$$f; \
	  fi; \
	done

uninstall-local:
	@set -x; list='$(MODOBJS)'; for p in $$list; do \
	  f=$(am__strip_dir) \
	  rm -f "$(DESTDIR)$(pkglibdir)/$$f"; \
	done

# noinst_HEADERS = 

EXTRA_DIST = $(MODSRCS)

CLEANFILES = $(MODOBJS)

##############################################################################
#
# Sources and Headers for libdispatch.

SRCS:=DODS.cc DODSCgi.cc DODSGlobalIQ.cc \
	DODSGlobalInit.cc DODSInitList.cc DODSLog.cc \
	DODSKeys.cc TheDODSKeys.cc \
	DODSTokenizer.cc \
	DODSContainer.cc DODSStatus.cc \
	DODSContainerPersistence.cc DODSContainerPersistenceCGI.cc \
	DODSContainerPersistenceFile.cc DODSContainerPersistenceList.cc \
	DODSContainerPersistenceVolatile.cc \
	DASResponseHandler.cc DDSResponseHandler.cc \
	DDXResponseHandler.cc ProcIdResponseHandler.cc SetResponseHandler.cc \
	DODSResponseHandler.cc DataResponseHandler.cc HelpResponseHandler.cc \
	StatusResponseHandler.cc DefineResponseHandler.cc \
	VersionResponseHandler.cc \
	ContainersResponseHandler.cc DefinitionsResponseHandler.cc \
	DeleteResponseHandler.cc KeysResponseHandler.cc \
	DODSResponseHandlerList.cc \
	DODSInfo.cc DODSTextInfo.cc DODSHTMLInfo.cc \
	DODSRequestHandler.cc DODSRequestHandlerList.cc \
	DODSBasicTransmitter.cc \
	DODSBasicHttpTransmitter.cc DODSFilterTransmitter.cc \
	DODSReporterList.cc DODSBasicException.cc \
	DODSMemoryException.cc DODSAutoPtr.cc DODSConstraintFuncs.cc \
	DODSHeapManager.cc DODSMemoryGlobalArea.cc \
	DODSMemoryManager.cc DODSProcessEncodedString.cc \
	OPeNDAPAggFactory.cc \
	DODSReturnManager.cc \
	DODSDefineList.cc \
	OPeNDAPBaseApp.cc

HDRS:=DODS.h DODSCgi.h DODSGlobalIQ.h \
	DODSGlobalInit.h DODSInitFuns.h DODSInitList.h DODSInitOrder.h \
	DODSInitializer.h DODSLog.h DODSLogException.h \
	DODSKeys.h TheDODSKeys.h DODSKeysException.h \
	DODSStatus.h \
	DODSParserException.h DODSTokenizer.h DODSContainer.h \
	DODSContainerPersistence.h DODSTransmitException.h \
	DODSContainerPersistenceCGI.h DODSContainerPersistenceFile.h \
	DODSContainerPersistenceList.h \
	DODSContainerPersistenceException.h \
	DODSContainerPersistenceVolatile.h \
	DASResponseHandler.h DDSResponseHandler.h \
	DDXResponseHandler.h ProcIdResponseHandler.h SetResponseHandler.h \
	DODSResponseHandler.h DataResponseHandler.h HelpResponseHandler.h \
	StatusResponseHandler.h DefineResponseHandler.h \
	VersionResponseHandler.h DODSResponseHandlerList.h \
	ContainersResponseHandler.h DefinitionsResponseHandler.h \
	DeleteResponseHandler.h \
	KeysResponseHandler.h \
	DODSResponseNames.h DODSResponseException.h \
	DODSInfo.h DODSTextInfo.h DODSHTMLInfo.h DODSRequestHandler.h \
	DODSRequestHandlerList.h \
	DODSIncorrectRequestException.h DODSDataHandlerInterface.h \
	DODSDataRequestInterface.h DODSTransmitter.h \
	DODSBasicHttpTransmitter.h DODSBasicTransmitter.h \
	DODSFilterTransmitter.h DODSReporter.h DODSReporterList.h \
	DODSBasicException.h DODSException.h \
	DODSHandlerException.h DODSAggregationException.h \
	DODSMemoryException.h \
	DODSAutoPtr.cc DODSAutoPtr.h \
	DODSConstraintFuncs.h DODSHeapManager.h \
	DODSMemoryGlobalArea.h DODSMemoryManager.h \
	DODSProcessEncodedString.h \
	DODSServerSystemResources.h DODSStatusReturn.h dispatch_version.h \
	OPeNDAPAggFactory.h DODSAggregationServer.h \
	DODSReturnManager.h \
	DODSDefineList.h DODSDefine.h \
	OPeNDAPDataNames.h OPeNDAPApp.h OPeNDAPObj.h OPeNDAPBaseApp.h

MODSRCS:=dods_module.cc cgi_persistence.cc file_persistence.cc \
	http_transmitter.cc

MODOBJS:=dods_module.o cgi_persistence.o file_persistence.o \
	http_transmitter.o

# TESTSRC:=dispatch_test.cc
# TESTEXEC:=dispatch_test

# $(TESTEXEC) : $(LIB) $(TESTOBJ) dods_module.o file_persistence.o \
# 	../cedar-handler/cedar_module.o \
# 	../cedar-handler/cedar_persistence.o \
# 	../cedar-handler/libcedar-handler.a  \
# 	$(CXX) -o $(TESTEXEC) $(LDFLAGS) $(TESTOBJ) dods_module.o \
# 		file_persistence.o \
# 		../cedar-handler/cedar_module.o \
# 		../cedar-handler/cedar_persistence.o $(LIBS)
