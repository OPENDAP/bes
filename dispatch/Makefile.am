
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
pkglib_LTLIBRARIES = libdap_module.la

libdispatch_la_SOURCES = $(SRCS) $(HDRS)
libdispatch_la_LDFLAGS = -version-info $(LIBDISPATCH_VERSION)

libdap_module_la_SOURCES = $(DAP_SRCS) $(DAP_HDRS)
libdap_module_la_LDFLAGS = -version-info $(LIBDISPATCH_VERSION)

pkginclude_HEADERS = $(HDRS) $(DAP_HDRS)

DISPATCH_INSTALL_FILES=dispatch_help.html dispatch_help.txt
install-data-local: $(DISPATCH_INSTALL_FILES)
	test -z "$(sysconfdir)/bes" || $(mkdir_p) "$(DESTDIR)$(sysconfdir)/bes"
	list='$(DISPATCH_INSTALL_FILES)'; for p in $$list; do \
	  if test -f $$p; then \
	    f=$(am__strip_dir) \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(sysconfdir)/bes/$$f; \
	  fi; \
	done

uninstall-local:
	@set -x; list='$(DISPATCH_INSTALL_FILES)'; for p in $$list; do \
	  f=$(am__strip_dir) \
	  rm -f "$(DESTDIR)$(sysconfdir)/bes/$$f"; \
	done

##############################################################################
#
# Sources and Headers for libdispatch.

SRCS:=BESInterface.cc BESCgiInterface.cc BESGlobalIQ.cc \
	BESGlobalInit.cc BESInitList.cc BESLog.cc BESKeys.cc \
	TheBESKeys.cc BESContainer.cc BESStatus.cc \
	BESContainerStorageCatalog.cc BESContainerStorageFile.cc \
	BESContainerStorageList.cc BESContainerStorageVolatile.cc \
	BESProcIdResponseHandler.cc BESResponseHandler.cc \
	BESHelpResponseHandler.cc BESStatusResponseHandler.cc \
	BESVersionResponseHandler.cc BESKeysResponseHandler.cc \
	BESResponseHandlerList.cc \
	BESCatalogDirectory.cc BESCatalogList.cc \
	BESInfo.cc BESTextInfo.cc BESVersionInfo.cc BESHTMLInfo.cc \
	BESXMLInfo.cc BESSilentInfo.cc \
	BESRequestHandler.cc BESRequestHandlerList.cc \
	BESBasicTransmitter.cc \
	BESBasicHttpTransmitter.cc BESFilterTransmitter.cc \
	BESReporterList.cc BESBasicException.cc \
	BESMemoryException.cc BESAutoPtr.cc BESConstraintFuncs.cc \
	BESHeapManager.cc BESMemoryGlobalArea.cc \
	BESMemoryManager.cc BESProcessEncodedString.cc \
	BESAggFactory.cc \
	BESReturnManager.cc \
	BESDefinitionStorageList.cc \
	BESDefinitionStorageVolatile.cc \
	BESBaseApp.cc BESModuleApp.cc default_module.cc

HDRS:=BESInterface.h BESCgiInterface.h BESGlobalIQ.h \
	BESGlobalInit.h BESInitFuns.h BESInitList.h BESInitOrder.h \
	BESInitializer.h BESLog.h BESLogException.h \
	BESKeys.h TheBESKeys.h BESKeysException.h \
	BESStatus.h \
	BESContainer.h \
	BESContainerStorage.h BESTransmitException.h \
	BESContainerStorageCatalog.h BESContainerStorageFile.h \
	BESContainerStorageList.h \
	BESContainerStorageException.h \
	BESContainerStorageVolatile.h \
	BESProcIdResponseHandler.h BESResponseHandler.h BESHelpResponseHandler.h \
	BESStatusResponseHandler.h BESVersionResponseHandler.h \
	BESResponseHandlerList.h BESKeysResponseHandler.h \
	BESCatalogDirectory.h BESCatalog.h \
	BESCatalogList.h \
	BESResponseNames.h BESResponseException.h \
	BESInfo.h BESTextInfo.h BESHTMLInfo.h BESVersionInfo.h \
	BESXMLInfo.h BESSilentInfo.h \
	BESRequestHandler.h \
	BESRequestHandlerList.h \
	BESIncorrectRequestException.h BESDataHandlerInterface.h \
	BESDataRequestInterface.h BESTransmitter.h \
	BESBasicHttpTransmitter.h BESBasicTransmitter.h \
	BESFilterTransmitter.h BESReporter.h BESReporterList.h \
	BESBasicException.h BESException.h \
	BESHandlerException.h BESAggregationException.h \
	BESMemoryException.h \
	BESAutoPtr.cc BESAutoPtr.h \
	BESConstraintFuncs.h BESHeapManager.h \
	BESMemoryGlobalArea.h BESMemoryManager.h \
	BESProcessEncodedString.h \
	BESServerSystemResources.h BESStatusReturn.h dispatch_version.h \
	BESAggFactory.h BESAggregationServer.h \
	BESReturnManager.h \
	BESDefine.h BESDefinitionStorageList.h \
	BESDefinitionStorageException.h BESDefinitionStorage.h \
	BESDefinitionStorageVolatile.h \
	BESDataNames.h BESApp.h BESObj.h BESBaseApp.h \
	BESAbstractModule.h BESPluginFactory.h BESPlugin.h \
	BESPluginException.h default_module.h BESTransmitterNames.h \
	BESModuleApp.h

DAP_SRCS:=BESCatalogResponseHandler.cc BESShowContainersResponseHandler.cc \
	BESDASResponseHandler.cc BESDDSResponseHandler.cc BESDDXResponseHandler.cc \
	BESDataResponseHandler.cc BESDefineResponseHandler.cc \
	BESShowDefsResponseHandler.cc \
	BESDelContainerResponseHandler.cc BESDelContainersResponseHandler.cc \
	BESDelDefResponseHandler.cc BESDelDefsResponseHandler.cc \
	BESSetContainerResponseHandler.cc BESModule.cc

DAP_HDRS:=BESCatalogResponseHandler.h BESShowContainersResponseHandler.h \
	BESDASResponseHandler.h BESDDSResponseHandler.h BESDDXResponseHandler.h \
	BESDataResponseHandler.h BESDefineResponseHandler.h \
	BESShowDefsResponseHandler.h \
	BESDelContainerResponseHandler.h BESDelContainersResponseHandler.h \
	BESDelDefResponseHandler.h BESDelDefsResponseHandler.h \
	BESSetContainerResponseHandler.h BESModule.h
