// DODSProcessEncodedString.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSProcessEncodedString_h_
#define DODSProcessEncodedString_h_ 1

#include <map>
#include <string>
#include <iostream>

using std::map ;
using std::string ; 
using std::less ;
using std::cout ;
using std::endl ;

class DODSProcessEncodedString
{
    map<string,string,less<string> > _entries;
    string parseHex(const char *hexstr); 
    const unsigned int convertHex( const char* what );
public:
    DODSProcessEncodedString (const char *s);
    string get_key(const string& s); 
    void show_keys()
    {
	map<string,string,less<string> >::iterator i;
	for(i=_entries.begin(); i!=_entries.end(); ++i)
	    cout<<"key: "<<(*i).first<<", value: "<<(*i).second<<endl;
    }
};

#endif // DODSProcessEncodedString_h_

// $Log: DODSProcessEncodedString.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
