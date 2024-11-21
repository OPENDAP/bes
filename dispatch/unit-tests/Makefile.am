
# Tests

AUTOMAKE_OPTIONS = foreign

# Because cacheT and uncompressT don't work with distcheck and a
# parallel build. :-( jhrg 8/22/17
.NOTPARALLEL:

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dispatch $(DAP_CFLAGS)
LDADD = $(top_builddir)/dispatch/.libs/libbes_dispatch.a $(DAP_LIBS) $(BES_BZ2_LIBS) $(BES_ZLIB_LIBS) $(XML2_LIBS)

if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
LDADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of debugging.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align

AM_CXXFLAGS = -Wno-deprecated
AM_LDFLAGS =
include $(top_srcdir)/coverage.mk

check_PROGRAMS = $(TESTS)

noinst_HEADERS = test_config.h test_utils.h

BUILT_SOURCES = test_config.h bes.conf

if CPPUNIT
# This determines what gets run by 'make check.'
TESTS = keysT constraintT defT pfileT plistT pvolT replistT		\
reqhandlerT reqlistT resplistT infoT utilT regexT scrubT		\
checkT servicesT fsT urlT containerT uncompressT			\
BESCatalogListTest CatalogNodeTest CatalogItemTest \
ServerAdministratorTest kvp_utils_test \
RequestTimerTest BESFileLockingCacheTest FileCacheTest \
BesJsonLogTest

# removed cacheT jhrg 1/11/23
# FIXME keysT removed to see if it's the only blocker. jhrg 2/2/23

# Debugging code is compiled out of the project when
# BES_DEVELOPER is not set, and so running the debugT tests
# produces no useful result.
if BES_DEVELOPER
TESTS += debugT
endif

# This is a tool to look at CatalogEntry objects. jhrg 3.5.18
# complete_catalog_lister agglistT 

if LIBDAP
TESTS += catT
endif

else

TESTS =

check-local:
	@echo ""
	@echo "**********************************************************"
	@echo "You must have cppunit 1.12.x or greater installed to run *"
	@echo "check target in dispatch unit-tests directory            *"
	@echo "**********************************************************"
	@echo ""
endif

# Limit the files copied to just these, eliminating the files created when tests are run.
# jhrg 5/13/07
CACHE = cache/template.txt cache/testfile.txt.bz2 cache/testfile.txt.gz cache/testfile.txt.Z

EXTRA_DIST = $(CACHE) catalog_test catalog_test_baselines bad_keys1.ini \
	defT.ini empty.ini info_test.ini keys_test.ini \
	keys_test_include.ini keys_test_m1.ini \
	keys_test_m2.ini keys_test_m3.ini \
	keys_test_map.ini keys_test_map_map.ini keys_test_vector.ini \
	persistence_cgi_test.ini persistence_file_test.ini \
	persistence_mysql_test.ini container01.file \
	persistence_file1.txt persistence_file2.txt \
	persistence_file3.txt persistence_file4.txt \
	persistence_file_test.txt test_config.h.in \
	bes.conf.in

DISTCLEANFILES = 

CLEANFILES = *.log *.sum real* test_config.h bes.conf

############################################
# This was generating a directory of "." so 
# I migrated it to the abs_srcdir below 
# - ndp 4/12/17
#
#test_config.h: test_config.h.in Makefile
#	sed -e "s%[@]srcdir[@]%${srcdir}%" $< > test_config.h
#
# Changed test_config.h.in tp $(srcdir)/test_config.h.in. jhrg 1/21/18
test_config.h: $(srcdir)/test_config.h.in Makefile
	@mod_abs_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`; \
	mod_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	mod_abs_builddir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_builddir}'))"`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
		-e "s%[@]abs_top_srcdir[@]%$${mod_abs_top_srcdir}%" \
		-e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > test_config.h

bes.conf: $(srcdir)/bes.conf.in Makefile
	@mod_abs_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_srcdir}'))"`; \
	mod_abs_top_srcdir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	mod_abs_builddir=`${PYTHON} -c "import os.path; print(os.path.abspath('${abs_builddir}'))"`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
		-e "s%[@]abs_top_srcdir[@]%$${mod_abs_top_srcdir}%" \
		-e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > bes.conf

clean-local:
	cd $(srcdir)/cache && rm -f *_cache*
	rm -rf test_cache_64
	rm -rf testdir

############################################################################

keysT_SOURCES = keysT.cc

kvp_utils_test_SOURCES = kvp_utils_test.cc

constraintT_SOURCES = constraintT.cc

containerT_SOURCES = containerT.cc

defT_SOURCES = defT.cc

pfileT_SOURCES = pfileT.cc

plistT_SOURCES = plistT.cc

pvolT_SOURCES = pvolT.cc

replistT_SOURCES = replistT.cc TestReporter.cc TestReporter.h

reqhandlerT_SOURCES = reqhandlerT.cc TestRequestHandler.cc TestRequestHandler.h

reqlistT_SOURCES = reqlistT.cc TestRequestHandler.cc TestRequestHandler.h

resplistT_SOURCES = resplistT.cc TestResponseHandler.cc TestResponseHandler.h

infoT_SOURCES = infoT.cc

uncompressT_SOURCES = uncompressT.cc

if BES_DEVELOPER
debugT_SOURCES = debugT.cc
endif

if LIBDAP
catT_SOURCES = test_utils.cc catT.cc
endif

utilT_SOURCES = utilT.cc

urlT_SOURCES = urlT.cc

regexT_SOURCES = regexT.cc

scrubT_SOURCES = scrubT.cc

checkT_SOURCES = checkT.cc

CatalogNodeTest_SOURCES = test_utils.cc CatalogNodeTest.cc

CatalogItemTest_SOURCES = test_utils.cc CatalogItemTest.cc

servicesT_SOURCES = servicesT.cc

fsT_SOURCES = fsT.cc

# Added Unit test for singleton class BESCatalogList. ndp 5/6/2013
BESCatalogListTest_SOURCES  = BESCatalogListTest.cc

# complete_catalog_lister_SOURCES = complete_catalog_lister.cc
# complete_catalog_lister_OBJ = ../BESCatalogResponseHandler.o
# complete_catalog_lister_CPPFLAGS = $(AM_CPPFLAGS) $(XML2_CFLAGS)
# complete_catalog_lister_LDADD = $(complete_catalog_lister_OBJ) $(LDADD)

ServerAdministratorTest_SOURCES = ServerAdministratorTest.cc

RequestTimerTest_SOURCES = RequestTimerTest.cc

BESFileLockingCacheTest_SOURCES = BESFileLockingCacheTest.cc

FileCacheTest_SOURCES = FileCacheTest.cc
FileCacheTest_CPPFLAGS = $(AM_CPPFLAGS) $(OPENSSL_INC)
FileCacheTest_LDADD = $(LDADD) $(OPENSSL_LDFLAGS) $(OPENSSL_LIBS)

BesJsonLogTest_SOURCES = BesJsonLogTest.cc
BesJsonLogTest_CPPFLAGS = $(AM_CPPFLAGS)
BesJsonLogTest_LDADD = $(LDADD)
