
# Build libbes_dispatch and libdap_module, part of the BES code.

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = $(XML2_CFLAGS)

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

SUBDIRS = unit-tests
DIST_SUBDIRS = unit-tests

if LIBDAP
lib_LTLIBRARIES = libbes_dispatch.la libbes_dap.la
pkglib_LTLIBRARIES = libdap_module.la

bin_PROGRAMS = besregtest
else
lib_LTLIBRARIES = libbes_dispatch.la
endif

libbes_dispatch_la_SOURCES = $(SRCS) $(HDRS)
libbes_dispatch_la_LDFLAGS = -version-info $(LIBDISPATCH_VERSION)
libbes_dispatch_la_LIBADD = $(BES_ZLIB_LIBS) $(BES_BZ2_LIBS) $(BES_DL_LIBS)

libdap_module_la_SOURCES = $(DAP_SRCS) $(DAP_HDRS)
libdap_module_la_CPPFLAGS = $(DAP_CFLAGS) $(AM_CPPFLAGS)
libdap_module_la_LDFLAGS = -avoid-version -module
libdap_module_la_LIBADD = libbes_dispatch.la libbes_dap.la $(DAP_SERVER_LIBS)

libbes_dap_la_SOURCES = $(BES_DAP_SRCS) $(BES_DAP_HDRS)
libbes_dap_la_CPPFLAGS = $(DAP_CFLAGS) $(AM_CPPFLAGS)
libbes_dap_la_LDFLAGS = -version-info $(LIBBES_DAP_VERSION)
libbes_dap_la_LIBADD = libbes_dispatch.la $(DAP_SERVER_LIBS)

besregtest_SOURCES = besregtest.cc
besregtest_CPPFLAGS = $(AM_CPPFLAGS)
besregtest_LDADD = libbes_dispatch.la

pkginclude_HEADERS = $(HDRS) $(DAP_HDRS) $(BES_DAP_HDRS)

#nobase_sysconf_DATA = bes/bes.conf

pkgdata_DATA = bes/bes_help.html bes/bes_help.txt bes/bes_help.xml \
	bes/dap_help.html bes/dap_help.txt bes/dap_help.xml

EXTRA_DIST = bes/bes.conf.in bes/bes.conf bes/bes_help.html bes/bes_help.txt \
	bes/bes_help.xml bes-uncompress.sh.in bes-purge.pl.in \
	bes/dap.conf.in bes/dap_help.html bes/dap_help.txt \
	bes/dap_help.xml

DISTCLEANFILES = bes/bes.conf bes/dap.conf

if LIBDAP
install-data-local: bes/bes.conf bes/dap.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	test -f $(DESTDIR)$(sysconfdir)/bes/bes.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/bes.conf $(DESTDIR)$(sysconfdir)/bes/bes.conf.$$currdate; \
	$(INSTALL_DATA) bes/bes.conf $(DESTDIR)$(sysconfdir)/bes/bes.conf; \
	test -f $(DESTDIR)$(sysconfdir)/bes/modules/dap.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/modules/dap.conf $(DESTDIR)$(sysconfdir)/bes/modules/dap.conf.$$currdate; \
	$(INSTALL_DATA) bes/dap.conf $(DESTDIR)$(sysconfdir)/bes/modules/dap.conf
else
install-data-local: bes/bes.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -f $(DESTDIR)$(sysconfdir)/bes/bes.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/bes.conf $(DESTDIR)$(sysconfdir)/bes/bes.conf.$$currdate; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	$(INSTALL_DATA) bes/bes.conf $(DESTDIR)$(sysconfdir)/bes/bes.conf
endif

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/bes.conf $(DESTDIR)$(sysconfdir)/bes/modules/dap.conf

bes/bes.conf: bes/bes.conf.in ../config.status
	test -d bes || mkdir bes
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
		-e "s%[@]sysconfdir[@]%${sysconfdir}%" \
		-e "s%[@]pkglibdir[@]%${pkglibdir}%" \
		-e "s%[@]datarootdir[@]%${datarootdir}%" \
		-e "s%[@]datadir[@]%${datadir}%" \
		-e "s%[@]bindir[@]%${bindir}%" $< > bes/bes.conf

bes/dap.conf: bes/dap.conf.in ../config.status
	test -d bes || mkdir bes
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
		-e "s%[@]sysconfdir[@]%${sysconfdir}%" \
		-e "s%[@]pkglibdir[@]%${pkglibdir}%" \
		-e "s%[@]datarootdir[@]%${datarootdir}%" \
		-e "s%[@]datadir[@]%${datadir}%" \
		-e "s%[@]bindir[@]%${bindir}%" $< > bes/dap.conf

##############################################################################
#
# Sources and Headers

SRCS = BESInterface.cc BESBasicInterface.cc BESGlobalIQ.cc		\
	BESGlobalInit.cc BESInitList.cc BESLog.cc BESKeys.cc		\
	TheBESKeys.cc BESStatus.cc					\
	BESContainer.cc BESFileContainer.cc				\
	BESContainerStorage.cc BESContainerStorageFile.cc		\
	BESContainerStorageList.cc BESContainerStorageVolatile.cc	\
	BESSetContainerResponseHandler.cc BESShowContainersResponseHandler.cc \
	BESDelContainerResponseHandler.cc BESDelContainersResponseHandler.cc \
	BESDefine.cc BESDefinitionStorageList.cc			\
	BESDefinitionStorageVolatile.cc					\
	BESDefineResponseHandler.cc BESShowDefsResponseHandler.cc	\
	BESDelDefResponseHandler.cc BESDelDefsResponseHandler.cc	\
	BESSetContextResponseHandler.cc BESShowContextResponseHandler.cc \
	BESContextManager.cc						\
	BESProcIdResponseHandler.cc BESResponseHandler.cc		\
	BESHelpResponseHandler.cc BESStatusResponseHandler.cc		\
	BESVersionResponseHandler.cc BESConfigResponseHandler.cc	\
	BESStreamResponseHandler.cc BESResponseHandlerList.cc		\
	BESInfo.cc BESTextInfo.cc BESVersionInfo.cc BESHTMLInfo.cc	\
	BESInfoList.cc BESXMLInfo.cc BESSilentInfo.cc			\
	BESRequestHandler.cc BESRequestHandlerList.cc			\
	BESTransmitter.cc BESBasicTransmitter.cc BESBasicHttpTransmitter.cc \
	BESReporterList.cc BESConstraintFuncs.cc			\
	BESServiceRegistry.cc BESServicesResponseHandler.cc		\
	BESMemoryGlobalArea.cc						\
	BESMemoryManager.cc BESProcessEncodedString.cc			\
	BESAggFactory.cc BESAggregationServer.cc			\
	BESReturnManager.cc						\
	BESError.cc BESExceptionManager.cc				\
	BESDataHandlerInterface.cc					\
	BESIndent.cc BESBaseApp.cc BESModuleApp.cc BESUtil.cc BESStopWatch.cc \
	BESRegex.cc BESScrub.cc BESDebug.cc BESDefaultModule.cc		\
	BESCache.cc BESUncompressManager.cc BESUncompressGZ.cc		\
	BESUncompressBZ2.cc BESUncompressZ.cc BESTokenizer.cc		\
	BESFSDir.cc BESFSFile.cc

HDRS = BESInterface.h BESBasicInterface.h BESGlobalIQ.h 		\
	BESGlobalInit.h BESInitFuns.h BESInitList.h BESInitOrder.h 	\
	BESInitializer.h BESLog.h 					\
	BESKeys.h TheBESKeys.h 						\
	BESStatus.h 							\
	BESContainer.h BESFileContainer.h BESContainerStorage.h 	\
	BESContainerStorageFile.h BESContainerStorageList.h 		\
	BESContainerStorageVolatile.h 					\
	BESSetContainerResponseHandler.h BESShowContainersResponseHandler.h \
	BESDelContainerResponseHandler.h BESDelContainersResponseHandler.h \
	BESDefine.h BESDefinitionStorageList.h BESDefinitionStorage.h 	\
	BESDefinitionStorageVolatile.h 					\
	BESDefineResponseHandler.h BESShowDefsResponseHandler.h 	\
	BESDelDefResponseHandler.h BESDelDefsResponseHandler.h 		\
	BESSetContextResponseHandler.h BESShowContextResponseHandler.h 	\
	BESContextManager.h 						\
	BESProcIdResponseHandler.h BESResponseHandler.h 		\
	BESHelpResponseHandler.h BESStatusResponseHandler.h 		\
	BESVersionResponseHandler.h BESConfigResponseHandler.h 		\
	BESStreamResponseHandler.h BESResponseHandlerList.h 		\
	BESResponseNames.h 						\
	BESResponseObject.h BESInfo.h BESTextInfo.h BESHTMLInfo.h 	\
	BESVersionInfo.h BESInfoList.h BESXMLInfo.h BESSilentInfo.h 	\
	BESInfoNames.h 							\
	BESRequestHandler.h BESRequestHandlerList.h 			\
	BESDataHandlerInterface.h BESDataRequestInterface.h 		\
	BESTransmitter.h BESBasicHttpTransmitter.h BESBasicTransmitter.h \
	BESReporter.h BESReporterList.h 				\
	BESServiceRegistry.h BESServicesResponseHandler.h 		\
	BESError.h BESInternalError.h BESInternalFatalError.h 		\
	BESSyntaxUserError.h BESForbiddenError.h BESNotFoundError.h 	\
	BESConstraintFuncs.h BESProcessEncodedString.h 			\
	BESMemoryGlobalArea.h BESMemoryManager.h 			\
	BESServerSystemResources.h 					\
	BESAggFactory.h BESAggregationServer.h 				\
	BESReturnManager.h 						\
	BESDataNames.h BESApp.h BESObj.h BESIndent.h BESBaseApp.h 	\
	BESAbstractModule.h BESPluginFactory.h BESPlugin.h 		\
	BESDefaultModule.h BESTransmitterNames.h 			\
	BESExceptionManager.h 						\
	BESModuleApp.h BESUtil.h BESStopWatch.h BESRegex.h BESScrub.h 	\
	BESDebug.h BESCache.h BESUncompressManager.h 			\
	BESUncompressGZ.h BESUncompressBZ2.h BESUncompressZ.h		\
	BESTokenizer.h BESFSDir.h BESFSFile.h

DAP_SRCS = BESCatalogResponseHandler.cc \
	BESDASResponseHandler.cc BESDDSResponseHandler.cc \
	BESDDXResponseHandler.cc \
	BESDataResponseHandler.cc \
	BESDataDDXResponseHandler.cc \
	BESDapModule.cc \
	BESDapRequestHandler.cc BESDapTransmit.cc

DAP_HDRS = BESCatalogResponseHandler.h \
	BESDASResponseHandler.h BESDDSResponseHandler.h \
	BESDDXResponseHandler.h \
	BESDataResponseHandler.h \
	BESDataDDXResponseHandler.h \
	BESDapModule.h \
	BESDapRequestHandler.h BESDapTransmit.h

BES_DAP_SRCS = BESDapResponse.cc BESDASResponse.cc BESDDSResponse.cc \
	BESDataDDSResponse.cc \
	BESContainerStorageCatalog.cc BESCatalogDirectory.cc \
	BESCatalogUtils.cc BESCatalogList.cc BESDapError.cc \
	BESDapErrorInfo.cc BESDapService.cc

BES_DAP_HDRS = BESDapNames.h BESDapResponse.h BESDASResponse.h \
	BESDDSResponse.h \
	BESDataDDSResponse.h \
	BESContainerStorageCatalog.h \
	BESCatalogDirectory.h BESCatalog.h \
	BESCatalogUtils.h BESCatalogList.h BESDapError.h \
	BESDapErrorInfo.h BESDapService.h

C4_DB=$(C4_DIR)/dispatch.db
C4_HTML=$(C4_dir)/dispatch.html
C4_XM=$(C4_dir)/dispatch.xml

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	-mkdir $(C4_DIR)
	cccc --outdir=$(C4_DIR) \
		$(SRCS) $(DAP_SRCS) $(BES_DAP_SRCS) \
		$(HDRS) $(DAP_HDRS) $(BES_DAP_HDRS)
