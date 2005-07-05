// DODSDefine.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSDefine_h_
#define DODSDefine_h_ 1

#include <string>
#include <list>

using std::string ;
using std::list ;

#include "DODSContainer.h"

class DODSDefine
{
public:
    				DODSDefine() {}
    virtual			~DODSDefine() {}

    list<DODSContainer>		containers ;
    typedef list<DODSContainer>::iterator containers_iterator ;

    DODSDefine::containers_iterator first_container() { return containers.begin() ; }
    DODSDefine::containers_iterator end_container() { return containers.end() ; }

    string			aggregation_command ;
} ;

#endif // DODSDefine_h_

// $Log: DODSDefine.h,v $
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
