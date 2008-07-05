// ConnTestStrs.h

#ifndef E_ConnTestStrs_h
#define E_ConnTestStrs_h 1

#include <string>

using std::string ;

#include "PPTProtocol.h"

//class ConnTestStrs
//{
//public:
static string test_str[] = { (string)"This is a test",
			     (string)"define d as c with c.constraint=\"a[0,1,10]\";"
			   } ;

static string test_exp[] = { (string)"000000e"+"d"+"This is a test",
			     (string)"0000000"+"d",
			     (string)"000000fxvar1=val1;var2;",
			     (string)"000002cddefine d as c with c.constraint=\"a[0,1,10]\";",
			     (string)"0000000d"
			   } ;
//} ;

#endif // E_ConnTestStrs_h
