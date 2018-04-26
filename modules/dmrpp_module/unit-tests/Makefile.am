
# Tests

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I$(top_srcdir)/modules/dmrpp_module -I$(top_srcdir)/dispatch -I$(top_srcdir)/dap $(DAP_CFLAGS)

# Added -lz for ubuntu
LIBADD = $(BES_DISPATCH_LIB) $(BES_DAP_LIB) $(BES_EXTRA_LIBS) $(DAP_SERVER_LIBS) $(DAP_CLIENT_LIBS) -lz


if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
LIBADD += $(CPPUNIT_LIBS)
endif

if USE_VALGRIND
TESTS_ENVIRONMENT=valgrind --quiet --trace-children=yes --error-exitcode=1  --dsymutil=yes --leak-check=yes
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -Wcast-align
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

# This determines what gets built by make check
check_PROGRAMS = $(UNIT_TESTS)

# This determines what gets run by 'make check.'
TESTS = $(UNIT_TESTS)

noinst_HEADERS = test_config.h

DIRS_EXTRA = 

EXTRA_DIST = 

CLEANFILES = testout .dodsrc  *.gcda *.gcno test_config.h *.strm *.file tmp.txt

DISTCLEANFILES = 

BUILT_SOURCES = test_config.h

test_config.h: $(srcdir)/test_config.h.in Makefile
	@mod_abs_srcdir=`echo ${abs_srcdir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	mod_abs_top_srcdir=`echo ${abs_top_srcdir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	mod_abs_builddir=`echo ${abs_builddir} | sed 's%\(.*\)/\(.[^/]*\)/[.][.]%\1%g'`; \
	sed -e "s%[@]abs_srcdir[@]%$${mod_abs_srcdir}%" \
	    -e "s%[@]abs_top_srcdir[@]%$${mod_abs_top_srcdir}%" \
	    -e "s%[@]abs_builddir[@]%$${mod_abs_builddir}%" $< > test_config.h

############################################################################
# Unit Tests
#

if CPPUNIT
UNIT_TESTS = ChunkTest DmrppParserTest DmrppChunkedReadTest DmrppUtilTest
else
UNIT_TESTS =

check-local:
	@echo ""
	@echo "**********************************************************"
	@echo "You must have cppunit 1.12.x or greater installed to run *"
	@echo "check target in unit-tests directory                     *"
	@echo "**********************************************************"
	@echo ""
endif

# HDRS and SRCS are not used. They were used with VPATH, but that
# broke the distcheck target.

#HDRS = DmrppArray.h DmrppByte.h DmrppD4Enum.h DmrppD4Group.h DmrppD4Opaque.h\
#DmrppD4Sequence.h DmrppFloat32.h DmrppFloat64.h DmrppInt16.h DmrppInt32.h \
#DmrppInt64.h DmrppInt8.h DmrppParserSax2.h DmrppStr.h DmrppStructure.h \
#DmrppUInt16.h DmrppUInt32.h DmrppUInt64.h DmrppUrl.h DmrppUtil.h \
#DmrppTypeFactory.h DmrppCommon.h H4ByteStream.h

#SRCS = DmrppArray.cc DmrppByte.cc DmrppD4Enum.cc DmrppD4Group.cc DmrppD4Opaque.cc\
#DmrppD4Sequence.cc DmrppFloat32.cc DmrppFloat64.cc DmrppInt16.cc DmrppInt32.cc \
#DmrppInt64.cc DmrppInt8.cc DmrppParserSax2.cc DmrppStr.cc DmrppStructure.cc \
#DmrppUInt16.cc DmrppUInt32.cc DmrppUInt64.cc DmrppUrl.cc DmrppUtil.cc \
#DmrppTypeFactory.cc DmrppCommon.cc H4ByteStream.cc $(HDRS)

#OBJS = ../DmrppArray.o ../DmrppByte.o ../DmrppD4Enum.o ../DmrppD4Group.o ../DmrppD4Opaque.o\
#../DmrppD4Sequence.o ../DmrppFloat32.o ../DmrppFloat64.o ../DmrppInt16.o ../DmrppInt32.o \
#../DmrppInt64.o ../DmrppInt8.o ../DmrppParserSax2.o ../DmrppStr.o ../DmrppStructure.o \
#../DmrppUInt16.o ../DmrppUInt32.o ../DmrppUInt64.o ../DmrppUrl.o ../DmrppUtil.o \
#../DmrppTypeFactory.o ../DmrppCommon.o H4ByteStream.o 

OBJS = ../*.o 

ChunkTest_SOURCES = ChunkTest.cc
ChunkTest_LDADD = $(OBJS) $(LIBADD)

DmrppParserTest_SOURCES = DmrppParserTest.cc
DmrppParserTest_LDADD   =  $(OBJS) $(LIBADD) 

# DmrppTypeReadTest_SOURCES = DmrppTypeReadTest.cc
# DmrppTypeReadTest_LDADD   = $(OBJS) $(LIBADD)

DmrppChunkedReadTest_SOURCES = DmrppChunkedReadTest.cc
DmrppChunkedReadTest_LDADD   = $(OBJS) $(LIBADD)

# DmrppHttpReadTest_SOURCES = DmrppHttpReadTest.cc
# DmrppHttpReadTest_LDADD   = $(OBJS) $(LIBADD)

DmrppUtilTest_SOURCES = DmrppUtilTest.cc
DmrppUtilTest_LDADD   = $(OBJS) $(LIBADD)

