// DODSDefineList.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DODSDefineList_h
#define I_DODSDefineList_h 1

#include <map>
#include <string>

using std::map ;
using std::string ;

class DODSDefine ;
class DODSInfo ;

class DODSDefineList {
private:
    map< string, DODSDefine * > _def_list ;
    typedef map< string, DODSDefine * >::const_iterator Define_citer ;
    typedef map< string, DODSDefine * >::iterator Define_iter ;
public:
				DODSDefineList(void) {}
    virtual			~DODSDefineList(void) {}

    virtual bool		add_def( const string &def_name,
					 DODSDefine *def ) ;
    virtual bool		remove_def( const string &def_name ) ;
    virtual void		remove_defs( ) ;
    virtual DODSDefine *	find_def( const string &def_name ) ;
    virtual void		show_definitions( DODSInfo &info ) ;
};

#endif // I_DODSDefineList_h

// $Log: DODSDefineList.h,v $
// Revision 1.3  2005/03/17 19:25:29  pwest
// string parameters changed to const references. remove_def now deletes the definition and returns true if removed or false otherwise. Added method remove_defs to remove all definitions
//
// Revision 1.2  2005/03/15 19:55:36  pwest
// show containers and show definitions
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
