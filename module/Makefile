INCS = -I. -I../dispatch -I../../libdap
CXX = g++
CPPFLAGS =  $(DEFS) $(INCS)
CXXFLAGS = -gstabs -O2 -Wall -fPIC -O0
LDFLAGS = -shared -u main 
OBJS = 
LIBS = -L../dispatch/.libs -ldispatch -L../../libdap/.libs -ldap -lcurl -L/usr/kerberos/lib -lssl -lcrypto -ldl -lssl -lcrypto -lgssapi_krb5 -lkrb5 -lcom_err -lk5crypto -lresolv -ldl -lz -lz -L/usr/lib -lxml2 -lz -lpthread -lm -lpthread

all: OPeNDAPModule.so tryme

tryme: tryme.o $(OBJS)
	$(CXX) -o tryme -export-dynamic tryme.o $(OBJS) -ldl -L../dispatch/.libs -ldispatch

%.o : %.cc
	$(CXX) $(INCS) -g -O2 -c -o $@ $<

OPeNDAPModule.so: OPeNDAPModule.o
	$(CXX) $(LDFLAGS) -o OPeNDAPModule.so OPeNDAPModule.o $(LIBS)

OPeNDAPModule.o: OPeNDAPModule.cc
	$(CXX) -c -gstabs -O2 -Wall -fPIC -O0 $(INCS) -o $@ $<

clean:
	/bin/rm -f *.o *.so tryme

