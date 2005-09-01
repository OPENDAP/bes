// DODSContainerPersistenceException.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSContainerPersistenceException_h_
#define DODSContainerPersistenceException_h_ 1

#include "DODSBasicException.h"

/** @brief exception thrown if problems locating container information for a
 * symbolic name
 */
class DODSContainerPersistenceException:public DODSBasicException
{
public:
      			DODSContainerPersistenceException() {}
      			DODSContainerPersistenceException( const string &s )
			    : DODSBasicException( s ) {}
      virtual		~DODSContainerPersistenceException() {}
};

#endif // DODSContainerPersistenceException_h_

// $Log: DODSContainerPersistenceException.h,v $
// Revision 1.4  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.3  2004/12/15 17:36:01  pwest
//
// Changed the way in which the parser retrieves container information, going
// instead to ThePersistenceList, which goes through the list of container
// persistence instances it has.
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
