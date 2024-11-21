
# Build libbes_dispatch and libdap_module, part of the BES code.

AUTOMAKE_OPTIONS = foreign

# I added a new command to and put the ResponseHandler in xmlcommand to
# keep all of the files together. jhrg 2/8/18
AM_CPPFLAGS = -I$(top_srcdir)/dap -I$(top_srcdir)/xmlcommand $(DAP_CFLAGS) $(XML2_CFLAGS)

if BES_DEVELOPER
AM_CPPFLAGS += -DBES_DEVELOPER
endif

AM_CXXFLAGS=
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

SUBDIRS = . unit-tests tests

lib_LTLIBRARIES = libbes_dispatch.la 

libbes_dispatch_la_SOURCES = $(SRCS) $(HDRS) 
libbes_dispatch_la_LDFLAGS = -version-info $(LIBDISPATCH_VERSION)
libbes_dispatch_la_LIBADD = $(XML2_LIBS) $(DAP_LIBS) $(BES_ZLIB_LIBS)  $(BES_BZ2_LIBS) $(BES_DL_LIBS) $(PTHREAD_LIBS)

pkginclude_HEADERS = $(HDRS) 

pkgdata_DATA = bes/bes_help.html bes/bes_help.txt bes/bes_help.xml

EXTRA_DIST = bes/bes.conf.in bes/bes.conf bes/bes_help.html bes/bes_help.txt \
	bes/bes_help.xml PicoSHA2 bes/site.conf.proto

DISTCLEANFILES = bes/bes.conf 

# confdir = $(sysconfdir)/bes
# Edit while working on C7 RPM builds jhrg 9/19/18
confdir = $(sysconfdir)/bes
conf_DATA = bes/bes.conf  bes/site.conf.proto

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
	BESLog.cc BesJsonLog.cc TheBESKeys.cc			\
	kvp_utils.cc \
	BESContainer.cc BESFileContainer.cc				\
	BESContainerStorage.cc BESContainerStorageFile.cc		\
	BESContainerStorageList.cc BESContainerStorageVolatile.cc	\
	BESFileContainerStorage.cc \
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
	BESInfoList.cc BESXMLInfo.cc 			\
	BESRequestHandler.cc BESRequestHandlerList.cc			\
	BESTransmitter.cc \
	BESReporterList.cc BESConstraintFuncs.cc			\
	BESServiceRegistry.cc BESServicesResponseHandler.cc		\
	BESMemoryGlobalArea.cc						\
	BESMemoryManager.cc 			\
	BESReturnManager.cc						\
	BESError.cc				\
	BESDataHandlerInterface.cc					\
	BESIndent.cc BESApp.cc BESModuleApp.cc BESUtil.cc BESStopWatch.cc \
	BESRegex.cc BESScrub.cc BESDebug.cc BESDefaultModule.cc		\
	BESFileLockingCache.cc \
	BESUncompressCache.cc \
	BESUncompressManager3.cc \
	BESUncompress3GZ.cc BESUncompress3BZ2.cc BESUncompress3Z.cc \
	BESTokenizer.cc		\
	BESFSDir.cc BESFSFile.cc \
	BESCatalog.cc \
	BESCatalogDirectory.cc \
	BESCatalogUtils.cc \
	BESCatalogList.cc \
	BESCatalogEntry.cc \
	BESCatalogResponseHandler.cc ShowNodeResponseHandler.cc \
	CatalogNode.cc CatalogItem.cc \
	RequestServiceTimer.cc \
	ServerAdministrator.cc

#	BESAggFactory.cc BESAggregationServer.cc BESContainerStorageCatalog.cc

HDRS = BESInterface.h BESLog.h BesJsonLog.h			\
	TheBESKeys.h BESStatus.h 				\
	kvp_utils.h \
	BESNames.h \
	BESContainer.h BESFileContainer.h BESContainerStorage.h 	\
	BESContainerStorageFile.h BESContainerStorageList.h 		\
	BESContainerStorageVolatile.h 					\
	BESSetContainerResponseHandler.h BESShowContainersResponseHandler.h \
	BESShowErrorResponseHandler.h					\
	BESDelContainerResponseHandler.h BESDelContainersResponseHandler.h \
	BESDefine.h BESDefinitionStorageList.h BESDefinitionStorage.h 	\
	BESFileContainerStorage.h \
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
	BESVersionInfo.h BESInfoList.h BESXMLInfo.h  	\
	BESInfoNames.h 							\
	BESRequestHandler.h BESRequestHandlerList.h 			\
	BESDataHandlerInterface.h BESDataRequestInterface.h 		\
	BESTransmitter.h \
	BESReporter.h BESReporterList.h 				\
	BESServiceRegistry.h BESServicesResponseHandler.h 		\
	BESError.h BESInternalError.h BESInternalFatalError.h 		\
	BESSyntaxUserError.h BESForbiddenError.h BESNotFoundError.h 	\
	BESTimeoutError.h \
	BESConstraintFuncs.h  			\
	BESMemoryGlobalArea.h BESMemoryManager.h 			\
	BESServerSystemResources.h 					\
	BESReturnManager.h 						\
	BESDataNames.h BESApp.h BESObj.h BESIndent.h 	\
	BESAbstractModule.h BESPluginFactory.h BESPlugin.h 		\
	BESDefaultModule.h BESTransmitterNames.h 			\
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
	BESCatalogResponseHandler.h ShowNodeResponseHandler.h \
	CatalogNode.h CatalogItem.h \
	RequestServiceTimer.h \
	ServerAdministrator.h FileCache.h

#	BESAggFactory.h BESAggregationServer.h BESContainerStorageCatalog.h

C4_DB=$(C4_DIR)/dispatch.db
C4_HTML=$(C4_dir)/dispatch.html
C4_XM=$(C4_dir)/dispatch.xml

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	-mkdir $(C4_DIR)
	cccc --outdir=$(C4_DIR) \
		$(SRCS)$(HDRS)  
