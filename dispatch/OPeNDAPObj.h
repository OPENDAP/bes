// OPeNDAPObj.h

/** @brief top level OPeNDAP object to house generic methods
 */

#ifndef A_OPeNDAPObj_h
#define A_OPeNDAPObj_h 1

#include <iostream>

using std::ostream ;

class OPeNDAPObj
{
public:
    virtual void	dump( ostream &strm ) const = 0 ;
} ;

inline ostream &
operator<<( ostream &strm, const OPeNDAPObj &csac )
{
    csac.dump( strm ) ;
    return strm ;
}

#endif // A_OPeNDAPObj_h

// $Log: OPeNDAPObj.h,v $
