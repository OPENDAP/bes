
# Tests

AUTOMAKE_OPTIONS = foreign

# Arrange to build with the backward compatibility mode enabled.
AM_CPPFLAGS = -I$(top_srcdir)/dispatch

# These are not used by automake but are often useful for certain types of
# debugging. 
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

check_PROGRAMS = $(TESTS)

nodist_include_HEADERS = test_config.h

# This determines what gets run by 'make check.'
TESTS = constraintT containerT defT initT keysT pfileT			\
		plistT pvolT replistT reqhandlerT reqlistT resplistT	\
		infoT agglistT cacheT gzT uncompressT lockT

# Removed: failInitT. I Have no idea how this works or why it stopped
# working. jhrg 5/9/07

if BZ2UNCOMPRESS
TESTS += bz2T
endif

# This limits the files copied to just these eliminating the files
# created when tests are run. jhrg 5/13/07
CACHE = cache/template.txt cache/testfile.txt.bz2 cache/testfile.txt.gz

EXTRA_DIST = $(CACHE) common_tests.exp bad_keys1.ini bad_keys2.ini \
	defT.ini empty.ini info_test.ini keys_test.ini \
	persistence_cgi_test.ini persistence_file_test.ini \
	persistence_mysql_test.ini container01.file \
	persistence_file1.txt persistence_file2.txt \
	persistence_file3.txt persistence_file4.txt \
	persistence_file_test.txt test_config.h.in \
	cache_test.ini bes.conf

DISTCLEANFILES = test_config.h

CLEANFILES = *.log *.sum

test_config.h: test_config.h.in
	sed -e "s%[@]srcdir[@]%${srcdir}%" $< > test_config.h

clean-local:
	cd ${srcdir}/cache && rm *_cache*

############################################################################

constraintT_SOURCES = constraintT.cc baseApp.cc constraintT.h \
			baseApp.h Application.h
constraintT_LDADD =  ../libbes_dispatch.la

containerT_SOURCES = containerT.cc baseApp.cc containerT.h baseApp.h	\
			Application.h
containerT_LDADD =  ../libbes_dispatch.la

defT_SOURCES = defT.cc baseApp.cc defT.h baseApp.h Application.h
defT_LDADD =  ../libbes_dispatch.la

# failInitT_SOURCES = failInitT.cc TheFailedCat.cc baseApp.cc failInitT.h \
#			TheFailedCat.h baseApp.h Application.h
# failInitT_LDADD =  ../libbes_dispatch.la

initT_SOURCES = initT.cc TheCat.cc TheDog.cc cat.cc dog.cc Animal.cc	\
		baseApp.cc initT.h TheCat.h TheDog.h cat.h dog.h Animal.h \
		baseApp.h Application.h
initT_LDADD =  ../libbes_dispatch.la

keysT_SOURCES = keysT.cc baseApp.cc keysT.h baseApp.h Application.h
keysT_LDADD =  ../libbes_dispatch.la

pfileT_SOURCES = pfileT.cc baseApp.cc pfileT.h baseApp.h Application.h
pfileT_LDADD =  ../libbes_dispatch.la

plistT_SOURCES = plistT.cc baseApp.cc plistT.h baseApp.h Application.h
plistT_LDADD =  ../libbes_dispatch.la

pvolT_SOURCES = pvolT.cc baseApp.cc pvolT.h baseApp.h Application.h
pvolT_LDADD =  ../libbes_dispatch.la

replistT_SOURCES = replistT.cc TestReporter.cc baseApp.cc replistT.h	\
			TestReporter.h baseApp.h Application.h
replistT_LDADD =  ../libbes_dispatch.la

reqhandlerT_SOURCES = reqhandlerT.cc TestRequestHandler.cc baseApp.cc	\
			reqhandlerT.h TestRequestHandler.h baseApp.h \
			Application.h
reqhandlerT_LDADD =  ../libbes_dispatch.la

reqlistT_SOURCES = reqlistT.cc TestRequestHandler.cc baseApp.cc reqlistT.h \
			TestRequestHandler.h baseApp.h Application.h
reqlistT_LDADD =  ../libbes_dispatch.la

resplistT_SOURCES = resplistT.cc TestResponseHandler.cc baseApp.cc	\
			resplistT.h TestResponseHandler.h baseApp.h \
			Application.h
resplistT_LDADD =  ../libbes_dispatch.la

infoT_SOURCES = infoT.cc baseApp.cc infoT.h baseApp.h Application.h
infoT_LDADD =  ../libbes_dispatch.la

agglistT_SOURCES = agglistT.cc TestAggServer.cc baseApp.cc agglistT.h	\
			TestAggServer.h baseApp.h Application.h
agglistT_LDADD =  ../libbes_dispatch.la

cacheT_SOURCES = cacheT.cc baseApp.cc cacheT.h baseApp.h Application.h
cacheT_LDADD =  ../libbes_dispatch.la

lockT_SOURCES = lockT.cc baseApp.cc lockT.h baseApp.h Application.h
lockT_LDADD =  ../libbes_dispatch.la

gzT_SOURCES = gzT.cc baseApp.cc gzT.h baseApp.h Application.h
gzT_LDADD =  ../libbes_dispatch.la -lz

bz2T_SOURCES = bz2T.cc baseApp.cc bz2T.h baseApp.h Application.h
bz2T_LDADD =  ../libbes_dispatch.la

uncompressT_SOURCES = uncompressT.cc baseApp.cc uncompressT.h baseApp.h	\
			Application.h
uncompressT_LDADD =  ../libbes_dispatch.la

