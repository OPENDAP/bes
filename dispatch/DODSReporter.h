// DODSReporter.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef A_DODSReporter_h
#define A_DODSReporter_h 1

#include "DODSDataHandlerInterface.h"

class DODSReporter
{
protected:
			DODSReporter() {} ;
public:
    virtual		~DODSReporter() {} ;

    virtual void	report( const DODSDataHandlerInterface &dhi ) = 0 ;
} ;

#endif // A_DODSReporter_h

// $Log: DODSReporter.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
