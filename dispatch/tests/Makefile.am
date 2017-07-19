
AUTOMAKE_OPTIONS = foreign 

if USE_VALGRIND
TESTS_ENVIRONMENT=valgrind --trace-children=yes --error-exitcode=1 --dsymutil=yes \
--leak-check=yes  --max-stackframe=3000000  --quiet
endif

AM_CPPFLAGS = -I$(top_srcdir)
AM_CXXFLAGS =

# These are not used by automake but are often useful for certain types of
# debugging. The best way to use these is to run configure as:
#     export CXXFLAGS='...'; ./configure --disable-shared
# the --disable-shared is not required, but it seems to help with debuggers.
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

noinst_DATA =

CLEANFILES = 

EXTRA_DIST = 
DISTCLEANFILES = atconfig generate_data_baselines.sh generate_metadata_baselines.sh

SRCS = FileLockingCacheTest.cc 
HDRS = FileLockingCacheTest.h 
	
C4_DB=$(C4_DIR)/dispatch.db
C4_HTML=$(C4_dir)/dispatch.html
C4_XM=$(C4_dir)/dispatch.xml

C4_DIR=./cccc
.PHONY: cccc
cccc:	
	-mkdir $(C4_DIR)
	cccc --outdir=$(C4_DIR) \
		$(SRCS)$(HDRS)  
	