// DODSReporterList.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DODSReporterList_h
#define I_DODSReporterList_h 1

#include <map>
#include <string>

using std::map ;
using std::string ;

#include "DODSDataHandlerInterface.h"

class DODSReporter ;

class DODSReporterList {
private:
    map< string, DODSReporter * > _reporter_list ;
public:
				DODSReporterList(void) ;
    virtual			~DODSReporterList(void) ;

    typedef map< string, DODSReporter * >::const_iterator Reporter_citer ;
    typedef map< string, DODSReporter * >::iterator Reporter_iter ;

    virtual bool		add_reporter( string reporter_name,
					      DODSReporter * handler ) ;
    virtual DODSReporter *	remove_reporter( string reporter_name ) ;
    virtual DODSReporter *	find_reporter( string reporter_name ) ;

    virtual void		report( const DODSDataHandlerInterface &dhi ) ;
};

#endif // I_DODSReporterList_h

// $Log: DODSReporterList.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
