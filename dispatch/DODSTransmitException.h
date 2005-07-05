// DODSTransmitException.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSTransmitException_h_
#define DODSTransmitException_h_ 1

#include "DODSBasicException.h"

/** @brief exception thrown if problems loading keys from dods initialization
 * file.
 */
class DODSTransmitException: public DODSBasicException
{
public:
      			DODSTransmitException() {}
    			DODSTransmitException( const string &s )
			    { _description = s ; }
      virtual		~DODSTransmitException() {}
};

#endif // DODSTransmitException_h_

// $Log: DODSTransmitException.h,v $
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
