// DODSContainerPersistenceVolatile.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSContainerPersistenceVolatile_h_
#define DODSContainerPersistenceVolatile_h_ 1

#include <map>
#include <string>

using std::map ;
using std::string ;
using std::less ;
using std::allocator ;

#include "DODSContainerPersistence.h"

/** @brief implementation of DODSContainerPersistence that stores containers
 * for the duration of this process.
 *
 * This implementation of DODSContainerPersistence stores volatile
 * containers for the duration of this process. A list of containers is
 * stored in the object. The look_for method simply looks for the specified
 * symbolic name in the list of containers and returns if a match is found.
 * Containers can be added to this instance as long as the symbolic name
 * doesn't already exist.
 *
 * @see DODSContainerPersistence
 * @see DODSContainer
 */
class DODSContainerPersistenceVolatile : public DODSContainerPersistence
{
private:
    map< string, DODSContainer *, less< string >, allocator< string > > _container_list ;
public:
    				DODSContainerPersistenceVolatile( const string &n ) ;
    virtual			~DODSContainerPersistenceVolatile() ;

    typedef map< string, DODSContainer *, less< string >, allocator< string > >::const_iterator Container_citer ;
    typedef map< string, DODSContainer *, less< string >, allocator< string > >::iterator Container_iter ;
    virtual void		look_for( DODSContainer &d ) ;
    virtual void		add_container( string s_name, string r_name,
					       string type ) ;
    virtual bool		rem_container( const string &s_name ) ;

    virtual void		show_containers( DODSInfo &info ) ;
};

#endif // DODSContainerPersistenceVolatile_h_

// $Log: DODSContainerPersistenceVolatile.h,v $
// Revision 1.4  2005/03/17 19:23:58  pwest
// deleting the container in rem_container instead of returning the removed container, returning true if successfully removed and false otherwise
//
// Revision 1.3  2005/03/15 19:55:36  pwest
// show containers and show definitions
//
// Revision 1.2  2005/02/02 00:03:13  pwest
// ability to replace containers and definitions
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
