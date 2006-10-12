
# Build libbes_dispatch and libdap_module, part of the BES code.

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

lib_LTLIBRARIES = libbes_dispatch.la
pkglib_LTLIBRARIES = libdap_module.la

libbes_dispatch_la_SOURCES = $(SRCS) $(HDRS)
libbes_dispatch_la_LDFLAGS = -version-info $(LIBDISPATCH_VERSION)
libbes_dispatch_la_CPPFLAGS = -DBES_CONF_DIR=\"${sysconfdir}\"

libdap_module_la_SOURCES = $(DAP_SRCS) $(DAP_HDRS)
libdap_module_la_LDFLAGS = -avoid-version -module
libdap_module_la_LIBADD = libbes_dispatch.la

pkginclude_HEADERS = $(HDRS) $(DAP_HDRS)

nobase_sysconf_DATA = bes/bes.conf

pkgdata_DATA = bes/bes_help.html bes/bes_help.txt

dist_bin_SCRIPTS = bes-uncompress.sh bes-purge.pl

EXTRA_DIST = bes/bes.conf.in bes/bes.conf bes/bes_help.html bes/bes_help.txt \
	bes-uncompress.sh.in bes-purge.pl.in
DISTCLEANFILES = bes/bes.conf bes-uncompress.sh bes-purge.pl

bes/bes.conf: bes/bes.conf.in ../config.status
	test -d bes || mkdir bes
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
		-e "s%[@]sysconfdir[@]%${sysconfdir}%" \
		-e "s%[@]pkglibdir[@]%${pkglibdir}%" \
		-e "s%[@]bindir[@]%${bindir}%" $< > bes/bes.conf

bes-uncompress.sh: bes-uncompress.sh.in ../config.status
	sed -e "s%[@]GZIP[@]%${GZIP}%" \
		-e "s%[@]BZIP2[@]%${BZIP2}%" $< > bes-uncompress.sh

bes-purge.pl: bes-purge.pl.in ../config.status
	sed -e "s%[@]PERL[@]%${PERL}%" $< > bes-purge.pl

##############################################################################
#
# Sources and Headers

SRCS = BESInterface.cc BESCgiInterface.cc BESGlobalIQ.cc \
	BESGlobalInit.cc BESInitList.cc BESLog.cc BESKeys.cc \
	TheBESKeys.cc BESContainer.cc BESStatus.cc \
	BESContainerStorageCatalog.cc BESContainerStorageFile.cc \
	BESContainerStorageList.cc BESContainerStorageVolatile.cc \
	BESProcIdResponseHandler.cc BESResponseHandler.cc \
	BESHelpResponseHandler.cc BESStatusResponseHandler.cc \
	BESVersionResponseHandler.cc BESKeysResponseHandler.cc \
	BESFileResponseHandler.cc BESResponseHandlerList.cc \
	BESCatalogDirectory.cc BESCatalogList.cc \
	BESInfo.cc BESTextInfo.cc BESVersionInfo.cc BESHTMLInfo.cc \
	BESInfoList.cc \
	BESXMLInfo.cc BESSilentInfo.cc \
	BESRequestHandler.cc BESRequestHandlerList.cc \
	BESTransmitter.cc BESBasicTransmitter.cc \
	BESBasicHttpTransmitter.cc BESFilterTransmitter.cc \
	BESReporterList.cc BESAutoPtr.cc BESConstraintFuncs.cc \
	BESMemoryGlobalArea.cc \
	BESMemoryManager.cc BESProcessEncodedString.cc \
	BESAggFactory.cc \
	BESReturnManager.cc \
	BESDefinitionStorageList.cc \
	BESDefinitionStorageVolatile.cc \
	BESExceptionManager.cc \
	BESBaseApp.cc BESModuleApp.cc default_module.cc

HDRS = BESInterface.h BESCgiInterface.h BESGlobalIQ.h \
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
	BESFileResponseHandler.h BESCatalogDirectory.h BESCatalog.h \
	BESCatalogList.h \
	BESResponseNames.h BESResponseException.h \
	BESInfo.h BESTextInfo.h BESHTMLInfo.h BESVersionInfo.h BESInfoList.h \
	BESXMLInfo.h BESSilentInfo.h BESInfoNames.h \
	BESRequestHandler.h \
	BESRequestHandlerList.h \
	BESIncorrectRequestException.h BESDataHandlerInterface.h \
	BESDataRequestInterface.h BESTransmitter.h \
	BESBasicHttpTransmitter.h BESBasicTransmitter.h \
	BESFilterTransmitter.h BESReporter.h BESReporterList.h \
	BESException.h BESHandlerException.h BESAggregationException.h \
	BESMemoryException.h BESDatabaseException.h \
	BESAutoPtr.cc BESAutoPtr.h \
	BESConstraintFuncs.h \
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
	BESExceptionManager.h \
	BESModuleApp.h

DAP_SRCS = BESCatalogResponseHandler.cc BESShowContainersResponseHandler.cc \
	BESDASResponseHandler.cc BESDDSResponseHandler.cc \
	BESDDXResponseHandler.cc \
	BESDataResponseHandler.cc BESDefineResponseHandler.cc \
	BESShowDefsResponseHandler.cc \
	BESDelContainerResponseHandler.cc BESDelContainersResponseHandler.cc \
	BESDelDefResponseHandler.cc BESDelDefsResponseHandler.cc \
	BESSetContainerResponseHandler.cc BESDapModule.cc \
	BESDapTransmit.cc

DAP_HDRS = BESCatalogResponseHandler.h BESShowContainersResponseHandler.h \
	BESDASResponseHandler.h BESDDSResponseHandler.h \
	BESDDXResponseHandler.h \
	BESDataResponseHandler.h BESDefineResponseHandler.h \
	BESShowDefsResponseHandler.h \
	BESDelContainerResponseHandler.h BESDelContainersResponseHandler.h \
	BESDelDefResponseHandler.h BESDelDefsResponseHandler.h \
	BESSetContainerResponseHandler.h BESDapModule.h \
	BESDapTransmit.h
