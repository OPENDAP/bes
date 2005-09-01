// DODSContainerPersistenceMySQL.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSContainerPersistenceMySQL_h_
#define DODSContainerPersistenceMySQL_h_ 1

#include <string>

using std::string ;

#include "DODSContainerPersistence.h"

class DODSMySQLQuery ;

/** @brief persistent storage of containers in a MySQL database
 *
 * This implementation of DODSContainerPersistence looks up container
 * information in a MySQL database. The database is sepcified in the dods
 * initialization file using the keys:
 *
 * DODS.Container.Persistence.MySQL.&lt;name&gt;.server
 * DODS.Container.Persistence.MySQL.&lt;name&gt;.user
 * DODS.Container.Persistence.MySQL.&lt;name&gt;.password
 * DODS.Container.Persistence.MySQL.&lt;name&gt;.database
 *
 * where &lt;name&gt; is the name of this instance of the persistent store.
 *
 * The table name used is tbl_containers and has the following columns
 *
 * Table tbl_containers.
 * Defines the version of CEDARDB	
 * 
 * +---------------+-----------+------+-----+---------+-------+
 * | Field         | Type      | Null | Key | Default | Extra |
 * +---------------+-----------+------+-----+---------+-------+
 * | SYMBOLIC_NAME | char(250) |      | PRI |         |       |
 * | REAL_NAME     | char(250) |      |     |         |       |
 * | CONTAINER_TYPE| char(20)  |      |     |         |       |
 * +---------------+-----------+------+-----+---------+-------+
 *
 * @see DODSContainerPersistence
 * @see DODSContainer
 * @see DODSKeys
 */
class DODSContainerPersistenceMySQL : public DODSContainerPersistence
{
    DODSMySQLQuery *		_query ;
public:
    				DODSContainerPersistenceMySQL( const string &name ) ;
    virtual			~DODSContainerPersistenceMySQL();

    virtual void 		look_for( DODSContainer &d ) ;
    virtual void		add_container( string s_name, string r_name,
				       string type ) ;
    virtual bool		rem_container( const string &s_name ) ;

    virtual void		show_containers( DODSInfo &info ) ;
};

#endif // DODSContainerPersistenceMySQL_h_

// $Log: DODSContainerPersistenceMySQL.h,v $
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
