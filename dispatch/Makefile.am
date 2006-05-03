
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

SUBDIRS = test 
DIST_SUBDIRS = test

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
	DODSContainer.cc DODSStatus.cc \
	ContainerStorage.cc ContainerStorageCatalog.cc \
	ContainerStorageFile.cc ContainerStorageList.cc \
	ContainerStorageVolatile.cc \
	DASResponseHandler.cc DDSResponseHandler.cc \
	DDXResponseHandler.cc ProcIdResponseHandler.cc SetResponseHandler.cc \
	DODSResponseHandler.cc DataResponseHandler.cc HelpResponseHandler.cc \
	StatusResponseHandler.cc DefineResponseHandler.cc \
	VersionResponseHandler.cc \
	ContainersResponseHandler.cc DefinitionsResponseHandler.cc \
	DeleteResponseHandler.cc KeysResponseHandler.cc \
	CatalogResponseHandler.cc DirectoryCatalog.cc CatalogList.cc \
	DODSResponseHandlerList.cc \
	DODSInfo.cc DODSTextInfo.cc DODSVersionInfo.cc DODSHTMLInfo.cc \
	DODSXMLInfo.cc OPeNDAPSilentInfo.cc \
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
	OPeNDAPBaseApp.cc OPeNDAPModuleApp.cc dods_module.cc

HDRS:=DODS.h DODSCgi.h DODSGlobalIQ.h \
	DODSGlobalInit.h DODSInitFuns.h DODSInitList.h DODSInitOrder.h \
	DODSInitializer.h DODSLog.h DODSLogException.h \
	DODSKeys.h TheDODSKeys.h DODSKeysException.h \
	DODSStatus.h \
	DODSContainer.h \
	ContainerStorage.h DODSTransmitException.h \
	ContainerStorageCatalog.h ContainerStorageFile.h \
	ContainerStorageList.h \
	ContainerStorageException.h \
	ContainerStorageVolatile.h \
	DASResponseHandler.h DDSResponseHandler.h \
	DDXResponseHandler.h ProcIdResponseHandler.h SetResponseHandler.h \
	DODSResponseHandler.h DataResponseHandler.h HelpResponseHandler.h \
	StatusResponseHandler.h DefineResponseHandler.h \
	VersionResponseHandler.h DODSResponseHandlerList.h \
	ContainersResponseHandler.h DefinitionsResponseHandler.h \
	DeleteResponseHandler.h \
	KeysResponseHandler.h \
	CatalogResponseHandler.h DirectoryCatalog.h OPeNDAPCatalog.h \
	CatalogList.h \
	DODSResponseNames.h DODSResponseException.h \
	DODSInfo.h DODSTextInfo.h DODSHTMLInfo.h DODSVersionInfo.h \
	DODSXMLInfo.h OPeNDAPSilentInfo.h \
	DODSRequestHandler.h \
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
	OPeNDAPDataNames.h OPeNDAPApp.h OPeNDAPObj.h OPeNDAPBaseApp.h \
	OPeNDAPAbstractModule.h OPeNDAPPluginFactory.h OPeNDAPPlugin.h \
	OPeNDAPPluginException.h dods_module.h OPeNDAPTransmitterNames.h \
	OPeNDAPModuleApp.h

MODSRCS:=

MODOBJS:=

