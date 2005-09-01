// DODSAggregationException.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSAggregationException_h_
#define DODSAggregationException_h_ 1

#include "DODSBasicException.h"

/** @brief exception thrown if problems loading keys from dods initialization
 * file.
 */
class DODSAggregationException: public DODSBasicException
{
public:
      			DODSAggregationException() {}
    			DODSAggregationException( const string &s )
			    { _description = s ; }
      virtual		~DODSAggregationException() {}
};

#endif // DODSAggregationException_h_

// $Log: DODSAggregationException.h,v $
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
