
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

if LIBDAP
lib_LTLIBRARIES = libbes_dispatch.la libbes_dap.la
pkglib_LTLIBRARIES = libdap_module.la
else
lib_LTLIBRARIES = libbes_dispatch.la
endif

libbes_dispatch_la_SOURCES = $(SRCS) $(HDRS)
libbes_dispatch_la_LDFLAGS = -version-info $(LIBDISPATCH_VERSION)
# Defined here because configure does not have the value of $sysconfdir. jhrg
libbes_dispatch_la_CPPFLAGS = -DBES_CONF_DIR=\"${sysconfdir}/bes\"

libdap_module_la_SOURCES = $(DAP_SRCS) $(DAP_HDRS)
libdap_module_la_CPPFLAGS = $(DAP_CFLAGS)
libdap_module_la_LDFLAGS = -avoid-version -module
libdap_module_la_LIBADD = libbes_dispatch.la libbes_dap.la $(DAP_SERVER_LIBS)

libbes_dap_la_SOURCES = $(BES_DAP_SRCS) $(BES_DAP_HDRS)
libbes_dap_la_CPPFLAGS = $(DAP_CFLAGS)
libbes_dap_la_LDFLAGS = -version-info $(LIBBES_DAP_VERSION)

pkginclude_HEADERS = $(HDRS) $(DAP_HDRS) $(BES_DAP_HDRS)

nobase_sysconf_DATA = bes/bes.conf

pkgdata_DATA = bes/bes_help.html bes/bes_help.txt bes/dap_help.html bes/dap_help.txt

dist_bin_SCRIPTS = bes-uncompress.sh bes-purge.pl

EXTRA_DIST = bes/bes.conf.in bes/bes.conf bes/bes_help.html bes/bes_help.txt \
	bes-uncompress.sh.in bes-purge.pl.in bes/dap_help.html bes/dap_help.txt
DISTCLEANFILES = bes/bes.conf bes-uncompress.sh bes-purge.pl

bes/bes.conf: bes/bes.conf.in ../config.status
	test -d bes || mkdir bes
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
		-e "s%[@]sysconfdir[@]%${sysconfdir}%" \
		-e "s%[@]pkglibdir[@]%${pkglibdir}%" \
		-e "s%[@]datadir[@]%${datadir}%" \
		-e "s%[@]bindir[@]%${bindir}%" $< > bes/bes.conf

bes-uncompress.sh: bes-uncompress.sh.in ../config.status
	sed -e "s%[@]GZIP[@]%${GZIP}%" \
		-e "s%[@]BZIP2[@]%${BZIP2}%" $< > bes-uncompress.sh

bes-purge.pl: bes-purge.pl.in ../config.status
	sed -e "s%[@]PERL[@]%${PERL}%" $< > bes-purge.pl

##############################################################################
#
# Sources and Headers

SRCS = BESInterface.cc BESGlobalIQ.cc \
	BESGlobalInit.cc BESInitList.cc BESLog.cc BESKeys.cc \
	TheBESKeys.cc BESStatus.cc \
	BESContainer.cc \
	BESContainerStorageFile.cc \
	BESContainerStorageList.cc BESContainerStorageVolatile.cc \
	BESSetContainerResponseHandler.cc BESShowContainersResponseHandler.cc \
	BESDelContainerResponseHandler.cc BESDelContainersResponseHandler.cc \
	BESDefine.cc BESDefinitionStorageList.cc \
	BESDefinitionStorageVolatile.cc \
	BESDefineResponseHandler.cc BESShowDefsResponseHandler.cc \
	BESDelDefResponseHandler.cc BESDelDefsResponseHandler.cc \
	BESSetContextResponseHandler.cc BESShowContextResponseHandler.cc \
	BESContextManager.cc \
	BESProcIdResponseHandler.cc BESResponseHandler.cc \
	BESHelpResponseHandler.cc BESStatusResponseHandler.cc \
	BESVersionResponseHandler.cc BESKeysResponseHandler.cc \
	BESStreamResponseHandler.cc BESResponseHandlerList.cc \
	BESInfo.cc BESTextInfo.cc BESVersionInfo.cc BESHTMLInfo.cc \
	BESInfoList.cc BESXMLInfo.cc BESSilentInfo.cc \
	BESRequestHandler.cc BESRequestHandlerList.cc \
	BESTransmitter.cc BESBasicTransmitter.cc BESBasicHttpTransmitter.cc \
	BESReporterList.cc BESAutoPtr.cc BESConstraintFuncs.cc \
	BESMemoryGlobalArea.cc \
	BESMemoryManager.cc BESProcessEncodedString.cc \
	BESAggFactory.cc BESAggregationServer.cc \
	BESReturnManager.cc \
	BESException.cc BESExceptionManager.cc \
	BESDataHandlerInterface.cc \
	BESIndent.cc BESBaseApp.cc BESModuleApp.cc BESUtil.cc \
	BESDebug.cc BESDefaultModule.cc

HDRS = BESInterface.h BESGlobalIQ.h \
	BESGlobalInit.h BESInitFuns.h BESInitList.h BESInitOrder.h \
	BESInitializer.h BESLog.h BESLogException.h \
	BESKeys.h TheBESKeys.h BESKeysException.h \
	BESStatus.h \
	BESTransmitException.h \
	BESContainer.h BESContainerStorage.h \
	BESContainerStorageFile.h \
	BESContainerStorageList.h BESContainerStorageException.h \
	BESContainerStorageVolatile.h \
	BESSetContainerResponseHandler.h BESShowContainersResponseHandler.h \
	BESDelContainerResponseHandler.h BESDelContainersResponseHandler.h \
	BESDefine.h BESDefinitionStorageList.h \
	BESDefinitionStorageException.h BESDefinitionStorage.h \
	BESDefinitionStorageVolatile.h \
	BESDefineResponseHandler.h BESShowDefsResponseHandler.h \
	BESDelDefResponseHandler.h BESDelDefsResponseHandler.h \
	BESSetContextResponseHandler.h BESShowContextResponseHandler.h \
	BESContextManager.h \
	BESProcIdResponseHandler.h BESResponseHandler.h \
	BESHelpResponseHandler.h BESStatusResponseHandler.h \
	BESVersionResponseHandler.h BESKeysResponseHandler.h \
	BESStreamResponseHandler.h BESResponseHandlerList.h \
	BESResponseNames.h BESResponseException.h \
	BESResponseObject.h BESInfo.h BESTextInfo.h BESHTMLInfo.h \
	BESVersionInfo.h BESInfoList.h BESXMLInfo.h BESSilentInfo.h \
	BESInfoNames.h \
	BESRequestHandler.h BESRequestHandlerList.h \
	BESIncorrectRequestException.h BESDataHandlerInterface.h \
	BESDataRequestInterface.h BESTransmitter.h \
	BESBasicHttpTransmitter.h BESBasicTransmitter.h \
	BESReporter.h BESReporterList.h \
	BESException.h BESHandlerException.h BESAggregationException.h \
	BESMemoryException.h BESDatabaseException.h \
	BESAutoPtr.cc BESAutoPtr.h \
	BESConstraintFuncs.h \
	BESMemoryGlobalArea.h BESMemoryManager.h \
	BESProcessEncodedString.h \
	BESServerSystemResources.h BESStatusReturn.h \
	BESAggFactory.h BESAggregationServer.h \
	BESReturnManager.h \
	BESDataNames.h BESApp.h BESObj.h BESIndent.h BESBaseApp.h \
	BESAbstractModule.h BESPluginFactory.h BESPlugin.h \
	BESPluginException.h BESDefaultModule.h BESTransmitterNames.h \
	BESExceptionManager.h \
	BESModuleApp.h BESUtil.h BESDebug.h

DAP_SRCS = BESCatalogResponseHandler.cc \
	BESDASResponseHandler.cc BESDDSResponseHandler.cc \
	BESDDXResponseHandler.cc \
	BESDataResponseHandler.cc \
	BESDapModule.cc \
	BESDapRequestHandler.cc BESDapTransmit.cc

DAP_HDRS = BESCatalogResponseHandler.h \
	BESDASResponseHandler.h BESDDSResponseHandler.h \
	BESDDXResponseHandler.h \
	BESDataResponseHandler.h \
	BESDapModule.h \
	BESDapRequestHandler.h BESDapTransmit.h

BES_DAP_SRCS = BESDASResponse.cc BESDDSResponse.cc BESDataDDSResponse.cc \
	BESFilterTransmitter.cc BESCgiInterface.cc \
	BESContainerStorageCatalog.cc BESCatalogDirectory.cc \
	BESCatalogUtils.cc BESCatalogList.cc BESDapHandlerException.cc

BES_DAP_HDRS = BESDASResponse.h BESDDSResponse.h BESDataDDSResponse.h \
	BESFilterTransmitter.h BESCgiInterface.h \
	BESContainerStorageCatalog.h \
	BESCatalogDirectory.h BESCatalog.h \
	BESCatalogUtils.h BESCatalogList.h BESDapHandlerException.h

