
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

libdispatch_la_SOURCES = $(SRCS) $(HDRS)
libmodules_la_SOURCES = $(MODSRCS)

libdispatch_la_LDFLAGS = -version-info $(LIBDISPATCH_VERSION)
libmodules_la_LDFLAGS = -version-info $(LIBMODULES_VERSION)

pkginclude_HEADERS = $(HDRS)

# noinst_HEADERS = 

# EXTRA_DIST = 

# CLEANFILES = 

##############################################################################
#
# Sources and Headers for libdispatch.

SRCS:=DODS.cc DODSCgi.cc DODSGlobalIQ.cc \
	DODSGlobalInit.cc DODSInitList.cc DODSLog.cc TheDODSLog.cc \
	DODSKeys.cc TheDODSKeys.cc \
	DODSTestAuthenticate.cc DODSTokenizer.cc \
	DODSContainer.cc DODSStatus.cc \
	DODSContainerPersistence.cc DODSContainerPersistenceCGI.cc \
	DODSContainerPersistenceFile.cc DODSContainerPersistenceList.cc \
	ThePersistenceList.cc \
	DODSContainerPersistenceVolatile.cc \
	DASResponseHandler.cc DDSResponseHandler.cc \
	DDXResponseHandler.cc ProcIdResponseHandler.cc SetResponseHandler.cc \
	DODSResponseHandler.cc DataResponseHandler.cc HelpResponseHandler.cc \
	StatusResponseHandler.cc DefineResponseHandler.cc \
	VersionResponseHandler.cc \
	ContainersResponseHandler.cc DefinitionsResponseHandler.cc \
	DeleteResponseHandler.cc KeysResponseHandler.cc \
	DODSResponseHandlerList.cc TheResponseHandlerList.cc \
	DODSInfo.cc DODSTextInfo.cc DODSHTMLInfo.cc \
	DODSRequestHandler.cc DODSRequestHandlerList.cc \
	TheRequestHandlerList.cc DODSBasicTransmitter.cc \
	DODSBasicHttpTransmitter.cc DODSFilterTransmitter.cc \
	DODSReporterList.cc TheReporterList.cc DODSBasicException.cc \
	DODSMemoryException.cc DODSAutoPtr.cc DODSConstraintFuncs.cc \
	DODSEncode.cc DODSHeapManager.cc DODSMemoryGlobalArea.cc \
	DODSMemoryManager.cc DODSProcessEncodedString.cc \
	TheAggFactory.cc OPeNDAPAggFactory.cc \
	DODSReturnManager.cc TheDODSReturnManager.cc \
	DODSDefineList.cc TheDefineList.cc \
	OPeNDAPBaseApp.cc

HDRS:=DODS.h DODSCgi.h DODSGlobalIQ.h \
	DODSGlobalInit.h DODSInitFuns.h DODSInitList.h DODSInitOrder.h \
	DODSInitializer.h DODSLog.h TheDODSLog.h DODSLogException.h \
	DODSKeys.h TheDODSKeys.h DODSKeysException.h DODSAuthenticate.h \
	DODSTestAuthenticate.h DODSStatus.h \
	TheDODSAuthenticator.h DODSAuthenticateException.h \
	DODSParserException.h DODSTokenizer.h DODSContainer.h \
	DODSContainerPersistence.h DODSTransmitException.h \
	DODSContainerPersistenceCGI.h DODSContainerPersistenceFile.h \
	DODSContainerPersistenceList.h \
	ThePersistenceList.h DODSContainerPersistenceException.h \
	DODSContainerPersistenceVolatile.h \
	DASResponseHandler.h DDSResponseHandler.h \
	DDXResponseHandler.h ProcIdResponseHandler.h SetResponseHandler.h \
	DODSResponseHandler.h DataResponseHandler.h HelpResponseHandler.h \
	StatusResponseHandler.h DefineResponseHandler.h \
	VersionResponseHandler.h DODSResponseHandlerList.h \
	ContainersResponseHandler.h DefinitionsResponseHandler.h \
	DeleteResponseHandler.h TheResponseHandlerList.h \
	KeysResponseHandler.h \
	DODSResponseNames.h DODSResponseException.h \
	DODSInfo.h DODSTextInfo.h DODSHTMLInfo.h DODSRequestHandler.h \
	DODSRequestHandlerList.h TheRequestHandlerList.h \
	DODSIncorrectRequestException.h DODSDataHandlerInterface.h \
	DODSDataRequestInterface.h DODSTransmitter.h \
	DODSBasicHttpTransmitter.h DODSBasicTransmitter.h \
	DODSFilterTransmitter.h DODSReporter.h DODSReporterList.h \
	TheReporterList.h DODSBasicException.h DODSException.h \
	DODSHandlerException.h DODSAggregationException.h \
	DODSMemoryException.h \
	DODSAutoPtr.cc DODSAutoPtr.h \
	DODSConstraintFuncs.h DODSEncode.h DODSHeapManager.h \
	DODSMemoryGlobalArea.h DODSMemoryManager.h \
	DODSProcessEncodedString.h \
	DODSServerSystemResources.h DODSStatusReturn.h dispatch_version.h \
	TheAggFactory.h OPeNDAPAggFactory.h DODSAggregationServer.h \
	DODSReturnManager.h TheDODSReturnManager.h \
	DODSDefineList.h TheDefineList.h DODSDefine.h \
	DODSMySQLConnectException.h DODSMySQLQueryException.h \
	OPeNDAPDataNames.h OPeNDAPApp.h OPeNDAPObj.h OPeNDAPBaseApp.h

MODSRCS:=dods_module.cc cgi_persistence.cc file_persistence.cc \
	ini_authenticator.cc http_transmitter.cc

# test_authenticator.cc

# MYSQLSRCS:=DODSMySQLAuthenticate.cc DODSContainerPersistenceMySQL.cc \
# 	DODSMySQLChannel.cc DODSMySQLConnect.cc DODSMySQLQuery.cc \
# 	DODSMySQLResult.cc

# MYSQLHDRS:=DODSMySQLAuthenticate.h DODSContainerPersistenceMySQL.h \
# 	DODSMySQLChannel.h DODSMySQLConnect.h \
# 	DODSMySQLQuery.h DODSMySQLResult.h

# MYSQLMODSRCS:=mysql_persistence.cc mysql_authenticator.cc

# DISPATCHSRCS:=@DISPATCHSRCS@
# DISPATCHHDRS:=@DISPATCHHDRS@

# TESTSRC:=dispatch_test.cc
# TESTEXEC:=dispatch_test

# $(TESTEXEC) : $(LIB) $(TESTOBJ) dods_module.o file_persistence.o \
# 	test_authenticator.o ../cedar-handler/cedar_module.o \
# 	../cedar-handler/cedar_persistence.o \
# 	../cedar-handler/libcedar-handler.a  \
# 	$(CXX) -o $(TESTEXEC) $(LDFLAGS) $(TESTOBJ) dods_module.o \
# 		file_persistence.o test_authenticator.o \
# 		../cedar-handler/cedar_module.o \
# 		../cedar-handler/cedar_persistence.o $(LIBS)
