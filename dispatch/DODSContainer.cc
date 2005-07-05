// DODSContainer.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <iostream>

#include "DODSContainer.h"

DODSContainer::DODSContainer(const string &s)
    : _valid( false ),
      _real_name( "" ),
      _constraint( "" ),
      _symbolic_name( s ),
      _container_type( "" ),
      _attributes( "" )
{
}

DODSContainer::DODSContainer( const DODSContainer &copy_from )
    : _valid( copy_from._valid ),
      _real_name( copy_from._real_name ),
      _constraint( copy_from._constraint ),
      _symbolic_name( copy_from._symbolic_name ),
      _container_type( copy_from._container_type ),
      _attributes( copy_from._attributes )
{
}

// $Log: DODSContainer.cc,v $
// Revision 1.4  2005/04/19 17:57:58  pwest
// added copy constructor and default settings for variables
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
