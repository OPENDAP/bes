
# Build libbes_dispatch and libdap_module, part of the BES code.

AUTOMAKE_OPTIONS = foreign

# I added a new command to and put the ResponseHandler in xmlcommand to
# keep all of the files together. jhrg 2/8/18
AM_CPPFLAGS = -I$(top_srcdir)/xmlcommand $(XML2_CFLAGS) 
AM_CXXFLAGS = 

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

SUBDIRS = . unit-tests tests

lib_LTLIBRARIES = libbes_dispatch.la 

libbes_dispatch_la_SOURCES = $(SRCS) $(HDRS) 
libbes_dispatch_la_LDFLAGS = -version-info $(LIBDISPATCH_VERSION)
libbes_dispatch_la_LIBADD = $(XML2_LIBS) $(BES_ZLIB_LIBS) \
	$(BES_BZ2_LIBS) $(BES_DL_LIBS) $(PTHREAD_LIBS)

pkginclude_HEADERS = $(HDRS) 

pkgdata_DATA = bes/bes_help.html bes/bes_help.txt bes/bes_help.xml

EXTRA_DIST = bes/bes.conf.in bes/bes.conf bes/bes_help.html bes/bes_help.txt \
	bes/bes_help.xml PicoSHA2

DISTCLEANFILES = bes/bes.conf

install-data-local: bes/bes.conf
	@currdate=`date +"%y%m%d%H%M"`; \
	test -d $(DESTDIR)$(sysconfdir)/bes || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes; \
	test -f $(DESTDIR)$(sysconfdir)/bes/bes.conf && mv -f $(DESTDIR)$(sysconfdir)/bes/bes.conf $(DESTDIR)$(sysconfdir)/bes/bes.conf.$$currdate; \
	test -d $(DESTDIR)$(sysconfdir)/bes/modules || $(MKDIR_P) $(DESTDIR)$(sysconfdir)/bes/modules; \
	$(INSTALL_DATA) bes/bes.conf $(DESTDIR)$(sysconfdir)/bes/bes.conf

uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/bes/bes.conf

bes/bes.conf: bes/bes.conf.in ../config.status
	mkdir -p bes
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
		-e "s%[@]sysconfdir[@]%${sysconfdir}%" \
		-e "s%[@]pkglibdir[@]%${pkglibdir}%" \
		-e "s%[@]datarootdir[@]%${datarootdir}%" \
		-e "s%[@]datadir[@]%${datadir}%" \
		-e "s%[@]prefix[@]%${prefix}%" \
		-e "s%[@]bindir[@]%${bindir}%" $< > bes/bes.conf

##############################################################################
#
# Sources and Headers

SRCS = BESInterface.cc \
	BESLog.cc TheBESKeys.cc BESStatus.cc			\
	BESContainer.cc BESFileContainer.cc				\
	BESContainerStorage.cc BESContainerStorageFile.cc		\
	BESContainerStorageList.cc BESContainerStorageVolatile.cc	\
	BESContainerStorageCatalog.cc \
	BESSetContainerResponseHandler.cc BESShowContainersResponseHandler.cc \
	BESDelContainerResponseHandler.cc BESDelContainersResponseHandler.cc \
	BESShowErrorResponseHandler.cc					\
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
	BESTransmitter.cc BESBasicHttpTransmitter.cc \
	BESReporterList.cc BESConstraintFuncs.cc			\
	BESServiceRegistry.cc BESServicesResponseHandler.cc		\
	BESMemoryGlobalArea.cc						\
	BESMemoryManager.cc 			\
	BESAggFactory.cc BESAggregationServer.cc			\
	BESReturnManager.cc						\
	BESError.cc BESExceptionManager.cc				\
	BESDataHandlerInterface.cc					\
	BESIndent.cc BESApp.cc BESModuleApp.cc BESUtil.cc BESStopWatch.cc \
	BESRegex.cc BESScrub.cc BESDebug.cc BESDefaultModule.cc		\
	BESFileLockingCache.cc \
	BESUncompressCache.cc \
	BESUncompressManager3.cc \
	BESUncompress3GZ.cc BESUncompress3BZ2.cc BESUncompress3Z.cc \
	BESTokenizer.cc		\
	BESFSDir.cc BESFSFile.cc \
	BESCatalogDirectory.cc \
	BESCatalogUtils.cc \
	BESCatalogList.cc \
	BESCatalogEntry.cc \
	BESCatalogResponseHandler.cc
	
HDRS = BESInterface.h BESLog.h 			\
	TheBESKeys.h BESStatus.h 				\
	BESNames.h \
	BESContainer.h BESFileContainer.h BESContainerStorage.h 	\
	BESContainerStorageFile.h BESContainerStorageList.h 		\
	BESContainerStorageVolatile.h 					\
	BESSetContainerResponseHandler.h BESShowContainersResponseHandler.h \
	BESShowErrorResponseHandler.h					\
	BESDelContainerResponseHandler.h BESDelContainersResponseHandler.h \
	BESDefine.h BESDefinitionStorageList.h BESDefinitionStorage.h 	\
	BESContainerStorageCatalog.h \
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
	BESTransmitter.h BESBasicHttpTransmitter.h \
	BESReporter.h BESReporterList.h 				\
	BESServiceRegistry.h BESServicesResponseHandler.h 		\
	BESError.h BESInternalError.h BESInternalFatalError.h 		\
	BESSyntaxUserError.h BESForbiddenError.h BESNotFoundError.h 	\
	BESTimeoutError.h \
	BESConstraintFuncs.h  			\
	BESMemoryGlobalArea.h BESMemoryManager.h 			\
	BESServerSystemResources.h 					\
	BESAggFactory.h BESAggregationServer.h 				\
	BESReturnManager.h 						\
	BESDataNames.h BESApp.h BESObj.h BESIndent.h 	\
	BESAbstractModule.h BESPluginFactory.h BESPlugin.h 		\
	BESDefaultModule.h BESTransmitterNames.h 			\
	BESExceptionManager.h 						\
	BESModuleApp.h BESUtil.h BESStopWatch.h BESRegex.h BESScrub.h 	\
	BESDebug.h \
	BESFileLockingCache.h \
	BESUncompressCache.h \
	BESUncompressManager3.h \
	BESUncompress3BZ2.h BESUncompress3Z.h BESUncompress3GZ.h \
	BESTokenizer.h BESFSDir.h BESFSFile.h\
	BESCatalogDirectory.h \
	BESCatalog.h \
	BESCatalogUtils.h \
	BESCatalogList.h \
	BESCatalogEntry.h \
	BESCatalogResponseHandler.h
	
C4_DB=$(C4_DIR)/dispatch.db
C4_HTML=$(C4_dir)/dispatch.html
C4_XM=$(C4_dir)/dispatch.xml

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	-mkdir $(C4_DIR)
	cccc --outdir=$(C4_DIR) \
		$(SRCS)$(HDRS)  
