SRCS:=Animal.cc TestRequestHandler.cc TestResponseHandler.cc TheCat.cc \
	TheDog.cc TheFailedCat.cc baseApp.cc cat.cc dog.cc failInitT.cc \
	initT.cc pcgiT.cc pfileT.cc plistT.cc pmysqlT.cc reqhandlerT.cc \
	reqlistT.cc resplistT.cc containerT.cc replistT.cc TestReporter.cc \
	keysT.cc containerT.cc replistT.cc authtestT.cc encodeT.cc \
	tokenizerT.cc parserT.cc pvolT.cc constraintT.cc defT.cc

INCS:=-I. -I.. -I../../dap
DEFS:=-DHAVE_CONFIG_H
CXXFLAGS_DEBUG = -g3 -O0 -Wall -fno-default-inline -fno-defer-pop
#CXXFLAGS:=-gstabs -O2  -Wall -Wno-deprecated
CXXFLAGS:=$(CXXFLAGS_DEBUG)
CPPFLAGS:=$(CXXFLAGS) $(DEFS) $(INCS)

TESTS:=initT failInitT reqlistT reqhandlerT resplistT pfileT plistT pcgiT \
	pmysqlT keysT containerT replistT authtestT encodeT tokenizerT \
	parserT pvolT constraintT defT

INITOBJS:=TheCat.o TheDog.o baseApp.o cat.o dog.o Animal.o

LIB:=../libdispatch.a
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
	g++ -o defT defT.o baseApp.o $(LIBS) -L ../../dap -ldap++ -L../../../lib -L/usr/kerberos/lib -lrx -lz -lm -lnsl -lcurl -lcrypt -lssl

containerT : containerT.o baseApp.o $(LIB)
	g++ -o containerT containerT.o baseApp.o $(LIBS)

replistT : replistT.o TestReporter.o baseApp.o $(LIB)
	g++ -o replistT replistT.o TestReporter.o baseApp.o $(LIBS)

pfileT : pfileT.o baseApp.o $(LIB)
	g++ -o pfileT pfileT.o baseApp.o $(LIBS) -L ../../dap -ldap++ -L../../../lib -L/usr/kerberos/lib -lrx -lz -lm -lnsl -lcurl -lcrypt -lssl

pcgiT : pcgiT.o baseApp.o $(LIB)
	g++ -o pcgiT pcgiT.o baseApp.o $(LIBS) -L ../../dap -ldap++ -L../../../lib -L/usr/kerberos/lib -lrx -lz -lm -lnsl -lcurl -lcrypt -lssl

pmysqlT : pmysqlT.o baseApp.o $(LIB)
	g++ -o pmysqlT pmysqlT.o baseApp.o $(LIBS) -L../../dap -ldap++ /project/cedar/bin/mysql-4.0.18/lib/mysql/libmysqlclient.a -L../../../lib -lrx -lz -lm -lnsl -lcurl -L/usr/kerberos/lib -lcrypt -lssl

authtestT : authtestT.o baseApp.o $(LIB)
	g++ -o authtestT authtestT.o baseApp.o $(LIBS) -L../../../lib /project/cedar/bin/mysql-4.0.18/lib/mysql/libmysqlclient.a -lrx -lz -lm -lnsl -lcurl 

pvolT : pvolT.o baseApp.o $(LIB)
	g++ -o pvolT pvolT.o baseApp.o $(LIBS) -L ../../dap -ldap++ -L../../../lib -L/usr/kerberos/lib -lrx -lz -lm -lnsl -lcurl -lcrypt -lssl

plistT : plistT.o baseApp.o $(LIB)
	g++ -o plistT plistT.o baseApp.o $(LIBS) -L ../../dap -ldap++ -L../../../lib -L/usr/kerberos/lib -lrx -lz -lm -lnsl -lcurl -lcrypt -lssl

keysT : keysT.o baseApp.o $(LIB)
	g++ -o keysT keysT.o baseApp.o $(LIBS)

encodeT : encodeT.o baseApp.o $(LIB)
	g++ -o encodeT encodeT.o baseApp.o $(LIBS) -lcrypt

constraintT : constraintT.o baseApp.o $(LIB)
	g++ -o constraintT constraintT.o baseApp.o $(LIBS) -lcrypt

tokenizerT : tokenizerT.o baseApp.o $(LIB)
	g++ -o tokenizerT tokenizerT.o baseApp.o $(LIBS) -lcrypt

parserT : parserT.o baseApp.o ../dods_module.o $(LIB)
	g++ -o parserT parserT.o baseApp.o ../dods_module.o $(LIBS) -L ../../dap -ldap++ -L../../../lib -L/usr/kerberos/lib -lrx -lz -lm -lnsl -lcurl -lcrypt -lssl

.PHONY: depend
depend: 
	../../../etc/depend.sh -m Makefile -- $(CPPFLAGS) -- $(SRCS)

clean :
	/bin/rm -rf *.o $(TESTS) *.out

# DO NOT DELETE; depend requires this line
Animal.o: Animal.cc Animal.h
TestRequestHandler.o: TestRequestHandler.cc TestRequestHandler.h \
  ../DODSRequestHandler.h ../DODSDataHandlerInterface.h \
  ../DODSContainer.h
TestResponseHandler.o: TestResponseHandler.cc TestResponseHandler.h \
  ../DODSResponseHandler.h ../DODSDataHandlerInterface.h \
  ../DODSContainer.h ../DODSTransmitter.h
TheCat.o: TheCat.cc TheCat.h Animal.h cat.h ../DODSInitList.h \
  ../DODSGlobalInit.h ../DODSInitializer.h ../DODSInitFuns.h
TheDog.o: TheDog.cc TheDog.h Animal.h dog.h ../DODSInitList.h \
  ../DODSGlobalInit.h ../DODSInitializer.h ../DODSInitFuns.h
TheFailedCat.o: TheFailedCat.cc TheFailedCat.h Animal.h ../DODSInitList.h \
  ../DODSGlobalInit.h ../DODSInitializer.h ../DODSInitFuns.h
baseApp.o: baseApp.cc baseApp.h Application.h ../DODSGlobalIQ.h
cat.o: cat.cc cat.h Animal.h
dog.o: dog.cc dog.h Animal.h
failInitT.o: failInitT.cc failInitT.h baseApp.h Application.h
initT.o: initT.cc initT.h baseApp.h Application.h TheCat.h Animal.h \
  TheDog.h
pcgiT.o: pcgiT.cc pcgiT.h baseApp.h Application.h \
  ../DODSContainerPersistenceCGI.h ../DODSContainerPersistence.h \
  ../TheDODSKeys.h ../DODSKeys.h ../DODSException.h
pfileT.o: pfileT.cc pfileT.h baseApp.h Application.h \
  ../DODSContainerPersistenceFile.h ../DODSContainerPersistence.h \
  ../DODSException.h
plistT.o: plistT.cc plistT.h baseApp.h Application.h \
  ../DODSContainerPersistenceList.h ../DODSContainerPersistenceFile.h \
  ../DODSContainerPersistence.h
pmysqlT.o: pmysqlT.cc pmysqlT.h baseApp.h Application.h \
  ../DODSContainerPersistenceMySQL.h ../DODSContainerPersistence.h \
  ../TheDODSKeys.h ../DODSKeys.h ../DODSException.h
reqhandlerT.o: reqhandlerT.cc reqhandlerT.h baseApp.h Application.h \
  TestRequestHandler.h ../DODSRequestHandler.h \
  ../DODSDataHandlerInterface.h ../DODSContainer.h
reqlistT.o: reqlistT.cc reqlistT.h baseApp.h Application.h \
  ../DODSRequestHandlerList.h TestRequestHandler.h \
  ../DODSRequestHandler.h ../DODSDataHandlerInterface.h \
  ../DODSContainer.h
resplistT.o: resplistT.cc resplistT.h baseApp.h Application.h \
  ../DODSResponseHandlerList.h TestResponseHandler.h \
  ../DODSResponseHandler.h ../DODSDataHandlerInterface.h \
  ../DODSContainer.h ../DODSTransmitter.h
containerT.o: containerT.cc containerT.h baseApp.h Application.h \
  ../TheDODSKeys.h ../DODSKeys.h ../ThePersistenceList.h \
  ../DODSContainerPersistenceList.h ../DODSContainer.h \
  ../DODSContainerPersistenceFile.h ../DODSContainerPersistence.h \
  ../DODSException.h
replistT.o: replistT.cc replistT.h baseApp.h Application.h \
  ../DODSReporterList.h ../DODSDataHandlerInterface.h ../DODSContainer.h \
  TestReporter.h ../DODSReporter.h
TestReporter.o: TestReporter.cc TestReporter.h ../DODSReporter.h \
  ../DODSDataHandlerInterface.h ../DODSContainer.h
keysT.o: keysT.cc keysT.h baseApp.h Application.h ../DODSKeys.h \
  ../DODSException.h
containerT.o: containerT.cc containerT.h baseApp.h Application.h \
  ../TheDODSKeys.h ../DODSKeys.h ../ThePersistenceList.h \
  ../DODSContainerPersistenceList.h ../DODSContainer.h \
  ../DODSContainerPersistenceFile.h ../DODSContainerPersistence.h \
  ../DODSException.h
replistT.o: replistT.cc replistT.h baseApp.h Application.h \
  ../DODSReporterList.h ../DODSDataHandlerInterface.h ../DODSContainer.h \
  TestReporter.h ../DODSReporter.h
authtestT.o: authtestT.cc authtestT.h baseApp.h Application.h \
  ../DODSContainerPersistenceMySQL.h ../DODSContainerPersistence.h \
  ../TheDODSKeys.h ../DODSKeys.h ../DODSMySQLAuthenticate.h \
  ../DODSAuthenticate.h ../DODSDataHandlerInterface.h ../DODSContainer.h \
  ../DODSDataRequestInterface.h ../DODSException.h
encodeT.o: encodeT.cc encodeT.h baseApp.h Application.h ../DODSEncode.h \
  ../TheDODSKeys.h ../DODSKeys.h ../DODSMySQLAuthenticate.h \
  ../DODSAuthenticate.h ../DODSDataHandlerInterface.h ../DODSContainer.h \
  ../DODSDataRequestInterface.h ../DODSException.h
