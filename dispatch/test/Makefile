SRCS:=Animal.cc TestRequestHandler.cc TestResponseHandler.cc TheCat.cc \
	TheDog.cc TheFailedCat.cc baseApp.cc cat.cc dog.cc failInitT.cc \
	initT.cc pfileT.cc plistT.cc reqhandlerT.cc \
	reqlistT.cc resplistT.cc containerT.cc replistT.cc TestReporter.cc \
	keysT.cc containerT.cc replistT.cc \
	pvolT.cc constraintT.cc defT.cc

INCS:=-I. -I.. -I../../../libdap
DEFS:=-DHAVE_CONFIG_H
CXXFLAGS_DEBUG = -g3 -O0 -Wall -fno-default-inline -fno-defer-pop
#CXXFLAGS:=-gstabs -O2  -Wall -Wno-deprecated
CXXFLAGS:=$(CXXFLAGS_DEBUG)
CPPFLAGS:=$(CXXFLAGS) $(DEFS) $(INCS)

TESTS:=initT failInitT reqlistT reqhandlerT resplistT pfileT plistT \
	keysT containerT replistT pvolT constraintT defT

INITOBJS:=TheCat.o TheDog.o baseApp.o cat.o dog.o Animal.o

LIB:=../.libs/libdispatch.a
LIBS:=-L../.libs -ldispatch

all : $(TESTS)

%.o : %.cc
	g++ -c $(CPPFLAGS) $<

initT : $(INITOBJS) initT.o $(LIB)
	g++ -o initT initT.o $(INITOBJS) $(LIBS)

failInitT : $(INITOBJS) failInitT.o TheFailedCat.o $(LIB)
	g++ -o failInitT failInitT.o TheFailedCat.o $(INITOBJS) $(LIBS)

reqlistT : reqlistT.o baseApp.o TestRequestHandler.o $(LIB)
	g++ -o reqlistT reqlistT.o TestRequestHandler.o baseApp.o $(LIBS)

reqhandlerT : reqhandlerT.o baseApp.o TestRequestHandler.o $(LIB)
	g++ -o reqhandlerT reqhandlerT.o TestRequestHandler.o baseApp.o $(LIBS)

resplistT : resplistT.o baseApp.o TestResponseHandler.o $(LIB)
	g++ -o resplistT resplistT.o TestResponseHandler.o baseApp.o $(LIBS)

defT : defT.o baseApp.o $(LIB)
	g++ -o defT defT.o baseApp.o $(LIBS) -L../../../libdap/.libs -ldap -L../../../lib -L/usr/kerberos/lib -lz -lm -lnsl -lcurl -lcrypt -lssl

containerT : containerT.o baseApp.o $(LIB)
	g++ -o containerT containerT.o baseApp.o $(LIBS)

replistT : replistT.o TestReporter.o baseApp.o $(LIB)
	g++ -o replistT replistT.o TestReporter.o baseApp.o $(LIBS)

pfileT : pfileT.o baseApp.o $(LIB)
	g++ -o pfileT pfileT.o baseApp.o $(LIBS) -L../../../libdap/.libs -ldap -L../../../lib -L/usr/kerberos/lib -lz -lm -lnsl -lcurl -lcrypt -lssl

pvolT : pvolT.o baseApp.o $(LIB)
	g++ -o pvolT pvolT.o baseApp.o $(LIBS) -L../../../libdap/.libs -ldap -L../../../lib -L/usr/kerberos/lib -lz -lm -lnsl -lcurl -lcrypt -lssl

plistT : plistT.o baseApp.o $(LIB)
	g++ -o plistT plistT.o baseApp.o $(LIBS) -L../../../libdap/.libs -ldap -L../../../lib -L/usr/kerberos/lib -lz -lm -lnsl -lcurl -lcrypt -lssl

keysT : keysT.o baseApp.o $(LIB)
	g++ -o keysT keysT.o baseApp.o $(LIBS)

constraintT : constraintT.o baseApp.o $(LIB)
	g++ -o constraintT constraintT.o baseApp.o $(LIBS) -lcrypt

.PHONY: depend
depend: 
	../../../etc/depend.sh -m Makefile -- $(CPPFLAGS) -- $(SRCS)

clean :
	/bin/rm -rf *.o $(TESTS) *.out

# DO NOT DELETE; depend requires this line
