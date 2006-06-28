
# Tests

AUTOMAKE_OPTIONS = foreign

# Arrange to build with the backward compatibility mode enabled.
AM_CPPFLAGS = -I$(top_srcdir)/dispatch

# These are not used by automake but are often useful for certain types of
# debugging. 
CXXFLAGS_DEBUG = -g3 -O0 -fno-defer-pop -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

check_PROGRAMS = $(DEJAGNU_TESTS)

# This determines what gets run by 'make check.' The dejagnu_driver.sh 
# runs all the dejagnu test drivers by calling runtest 'by hand.'
TESTS = dejagnu_driver.sh

# Build the dejagnu test drivers as check programs; when automake fixes
# support for dejagnu so it can handle multiple drivers, add 'dejagnu to
# the automake options at the top of this file. See the section on dejagnu
# tests below. jhrg 7/6/05
dist_check_SCRIPTS = dejagnu_driver.sh

DIRS_EXTRA = testsuite

EXTRA_DIST = $(DIRS_EXTRA) common_tests.exp

CLEANFILES = *.log *.sum

############################################################################
# These tests use dejagnu. The drivers (passed to --tool) all use the
# subclassed types in Test*.cc and the TestTypesFactory.

# noinst_PROGRAMS above uses this variable.
DEJAGNU_TESTS = constraintT containerT defT failInitT initT keysT pfileT plistT pvolT replistT reqhandlerT reqlistT resplistT infoT

# Use this when automake adds support for multiple dejagnu test drivers. jhrg
# 6/30/05. 
# DEJATOOL = $(DEJAGNU_TESTS)
# RUNTESTDEFAULTFLAGS = --tool $$tool --srcdir $${tool}suite

constraintT_SOURCES = constraintT.cc baseApp.cc constraintT.h baseApp.h Application.h
constraintT_LDADD =  ../libdispatch.la

containerT_SOURCES = containerT.cc baseApp.cc containerT.h baseApp.h Application.h
containerT_LDADD =  ../libdispatch.la

defT_SOURCES = defT.cc baseApp.cc defT.h baseApp.h Application.h
defT_LDADD =  ../libdispatch.la

failInitT_SOURCES = failInitT.cc TheFailedCat.cc baseApp.cc failInitT.h TheFailedCat.h baseApp.h Application.h
failInitT_LDADD =  ../libdispatch.la

initT_SOURCES = initT.cc TheCat.cc TheDog.cc cat.cc dog.cc Animal.cc baseApp.cc initT.h TheCat.h TheDog.h cat.h dog.h Animal.h baseApp.h Application.h
initT_LDADD =  ../libdispatch.la

keysT_SOURCES = keysT.cc baseApp.cc keysT.h baseApp.h Application.h
keysT_LDADD =  ../libdispatch.la

pfileT_SOURCES = pfileT.cc baseApp.cc pfileT.h baseApp.h Application.h
pfileT_LDADD =  ../libdispatch.la

plistT_SOURCES = plistT.cc baseApp.cc plistT.h baseApp.h Application.h
plistT_LDADD =  ../libdispatch.la

pvolT_SOURCES = pvolT.cc baseApp.cc pvolT.h baseApp.h Application.h
pvolT_LDADD =  ../libdispatch.la

replistT_SOURCES = replistT.cc TestReporter.cc baseApp.cc replistT.h TestReporter.h baseApp.h Application.h
replistT_LDADD =  ../libdispatch.la

reqhandlerT_SOURCES = reqhandlerT.cc TestRequestHandler.cc baseApp.cc reqhandlerT.h TestRequestHandler.h baseApp.h Application.h
reqhandlerT_LDADD =  ../libdispatch.la

reqlistT_SOURCES = reqlistT.cc TestRequestHandler.cc baseApp.cc reqlistT.h TestRequestHandler.h baseApp.h Application.h
reqlistT_LDADD =  ../libdispatch.la

resplistT_SOURCES = resplistT.cc TestResponseHandler.cc baseApp.cc resplistT.h TestResponseHandler.h baseApp.h Application.h
resplistT_LDADD =  ../libdispatch.la

infoT_SOURCES = infoT.cc baseApp.cc infoT.h baseApp.h Application.h
infoT_LDADD =  ../libdispatch.la

