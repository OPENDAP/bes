
# Tests

AUTOMAKE_OPTIONS = foreign

# Arrange to build with the backward compatibility mode enabled.
AM_CPPFLAGS = -I$(top_srcdir)/dispatch
LDADD = ../libbes_dispatch.la $(XML2_LIBS)
if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
LDADD += $(CPPUNIT_LIBS)
endif
if LIBDAP
LDADD += ../libbes_dap.la
endif

# These are not used by automake but are often useful for certain types of
# debugging. 
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

check_PROGRAMS = $(TESTS)

#nodist_include_HEADERS = test_config.h breaks make rpm jhrg
noinst_HEADERS = test_config.h

if CPPUNIT
# This determines what gets run by 'make check.'
TESTS = constraintT containerT defT keysT pfileT			\
		plistT pvolT replistT reqhandlerT reqlistT resplistT	\
		infoT agglistT cacheT gzT zT uncompressT lockT		\
		encodeT debugT utilT regexT scrubT checkT servicesT fsT	\
		urlT

if LIBDAP
TESTS += catT
endif

if BZ2UNCOMPRESS
TESTS += bz2T
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

# This limits the files copied to just these eliminating the files
# created when tests are run. jhrg 5/13/07
CACHE = cache/template.txt cache/testfile.txt.bz2 cache/testfile.txt.gz cache/testfile.txt.Z

EXTRA_DIST = $(CACHE) common_tests.exp bad_keys1.ini \
	defT.ini empty.ini info_test.ini keys_test.ini \
	keys_test_include.ini keys_test_m1.ini \
	keys_test_m2.ini keys_test_m3.ini \
	persistence_cgi_test.ini persistence_file_test.ini \
	persistence_mysql_test.ini container01.file \
	persistence_file1.txt persistence_file2.txt \
	persistence_file3.txt persistence_file4.txt \
	persistence_file_test.txt test_config.h.in \
	cache_test.ini bes.conf

DISTCLEANFILES = test_config.h

CLEANFILES = *.log *.sum real*

test_config.h: test_config.h.in Makefile
	sed -e "s%[@]srcdir[@]%${srcdir}%" $< > test_config.h

clean-local:
	cd ${srcdir}/cache && rm -f *_cache*
	rm -rf ${srcdir}/test_cache_64
	rm -rf testdir

############################################################################

constraintT_SOURCES = constraintT.cc

containerT_SOURCES = containerT.cc

defT_SOURCES = defT.cc

keysT_SOURCES = keysT.cc

pfileT_SOURCES = pfileT.cc

plistT_SOURCES = plistT.cc

pvolT_SOURCES = pvolT.cc

replistT_SOURCES = replistT.cc TestReporter.cc TestReporter.h

reqhandlerT_SOURCES = reqhandlerT.cc TestRequestHandler.cc TestRequestHandler.h

reqlistT_SOURCES = reqlistT.cc TestRequestHandler.cc TestRequestHandler.h

resplistT_SOURCES = resplistT.cc TestResponseHandler.cc TestResponseHandler.h

infoT_SOURCES = infoT.cc
infoT_CPPFLAGS = $(AM_CPPFLAGS) $(XML2_CFLAGS)

agglistT_SOURCES = agglistT.cc TestAggServer.cc TestAggServer.h

cacheT_SOURCES = cacheT.cc

lockT_SOURCES = lockT.cc

gzT_SOURCES = gzT.cc
gzT_LDADD =  $(LDADD) -lz

bz2T_SOURCES = bz2T.cc

zT_SOURCES = zT.cc

uncompressT_SOURCES = uncompressT.cc

encodeT_SOURCES = encodeT.cc

debugT_SOURCES = debugT.cc

utilT_SOURCES = utilT.cc

urlT_SOURCES = urlT.cc

regexT_SOURCES = regexT.cc

scrubT_SOURCES = scrubT.cc

checkT_SOURCES = checkT.cc

servicesT_SOURCES = servicesT.cc
servicesT_CPPFLAGS = $(AM_CPPFLAGS) $(XML2_CFLAGS)

fsT_SOURCES = fsT.cc

if LIBDAP
catT_SOURCES = catT.cc ../BESCatalogResponseHandler.cc
catT_CPPFLAGS = $(AM_CPPFLAGS) $(XML2_CFLAGS)
endif

# BESCache3Test_SOURCES = BESCache3Test.cc
# BESCache3Test_CPPFLAGS = $(AM_CPPFLAGS)
# BESCache3Test_LDFLAGS = -lcrypto
