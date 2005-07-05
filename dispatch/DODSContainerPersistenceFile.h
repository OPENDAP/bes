// DODSContainerPersistenceFile.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DODSContainerPersistenceFile_h_
#define I_DODSContainerPersistenceFile_h_ 1

#include <string>
#include <map>

using std::string ;
using std::map ;
using std::less ;
using std::allocator ;

#include "DODSContainerPersistence.h"

/** @brief implementation of DODSContainerPersistence that represents a
 * way to read container information from a file.
 *
 * This impelementation of DODSContainerPersistence load container information
 * from a file. The name of the file is determined from the dods
 * initiailization file. The key is:
 *
 * DODS.Container.Persistence.File.<name>
 *
 * where <name> is the name of this persistent store.
 *
 * The format of the file is:
 *
 * <symbolic_name> <real_name> <data type>
 *
 * where the symbolic name is the symbolic name of the container, the
 * <real_name represents the physical location of the data, such as a file,
 * and the <data type> is the type of data being represented, such as netcdf,
 * cedar, etc...
 *
 * One container per line, can not span multiple lines
 *
 * @see DODSContainerPersistence
 * @see DODSContainer
 * @see DODSKeys
 */
class DODSContainerPersistenceFile : public DODSContainerPersistence
{
private:
    typedef struct _container
    {
	string _symbolic_name ;
	string _real_name ;
	string _container_type ;
    } container ;
    map< string, DODSContainerPersistenceFile::container *, less< string >, allocator< string > > _container_list ;
    typedef map< string, DODSContainerPersistenceFile::container *, less< string >, allocator< string > >::const_iterator Container_citer ;
    typedef map< string, DODSContainerPersistenceFile::container *, less< string >, allocator< string > >::iterator Container_iter ;

public:
    				DODSContainerPersistenceFile( const string &n );
    virtual			~DODSContainerPersistenceFile() ;

    virtual void		look_for( DODSContainer &d ) ;
    virtual void		add_container( string s_name, string r_ame,
					       string type ) ;
    virtual bool		rem_container( const string &s_name ) ;

    virtual void		show_containers( DODSInfo &info ) ;
};

#endif // I_DODSContainerPersistenceFile_h_

// $Log: DODSContainerPersistenceFile.h,v $
// Revision 1.8  2005/03/17 20:37:14  pwest
// implemented rem_container to remove the container from memory, but not from the file. Added documentation for rem_container and show_containers
//
// Revision 1.7  2005/03/17 19:23:58  pwest
// deleting the container in rem_container instead of returning the removed container, returning true if successfully removed and false otherwise
//
// Revision 1.6  2005/03/15 19:55:36  pwest
// show containers and show definitions
//
// Revision 1.5  2005/02/02 00:03:13  pwest
// ability to replace containers and definitions
//
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
