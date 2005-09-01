// DODSContainerPersistenceMySQL.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSContainerPersistenceMySQL.h"
#include "DODSContainer.h"
#include "DODSMySQLQuery.h"
#include "DODSContainerPersistenceException.h"
#include "DODSMemoryException.h"
#include "TheDODSKeys.h"
#include "DODSInfo.h"

/** @brief pull container information from the specified mysql database
 *
 * Constructs a DODSContainerPersistenceMySQL, pulling the mysql database
 * information from the dods initialization file and opening up a connection
 * to the MySQL database.
 *
 * The keys in the dods initialization file that represent the database are:
 *
 * DODS.Container.Persistence.MySQL.&lt;name&gt;.server
 * DODS.Container.Persistence.MySQL.&lt;name&gt;.user
 * DODS.Container.Persistence.MySQL.&lt;name&gt;.password
 * DODS.Container.Persistence.MySQL.&lt;name&gt;.database
 *
 * where &lt;name&gt; is the name of this instance of the persistent store.
 *
 * @param n name of this persistent store.
 * @throws DODSContainerPersistenceException if unable to retrieve the MySQL
 * database connection information from the dods initialization file.
 * @throws DODSMySQLConnectException if unable to connect to the MySQL
 * database.
 */
DODSContainerPersistenceMySQL::DODSContainerPersistenceMySQL( const string &n )
    :DODSContainerPersistence( n )
{
    bool found = false ;
    string my_key = "DODS.Container.Persistence.MySQL." + n + "." ;

    string my_server = TheDODSKeys->get_key( my_key + "server", found ) ;
    if( found == false )
    {
	DODSContainerPersistenceException pe;
	pe.set_error_description( "MySQL server not specified for " + n ) ;
	throw pe;
    }

    string my_user = TheDODSKeys->get_key( my_key + "user", found  ) ;
    if( found == false )
    {
	DODSContainerPersistenceException pe;
	pe.set_error_description( "MySQL user not specified for " + n ) ;
	throw pe;
    }

    string my_password = TheDODSKeys->get_key( my_key + "password", found  ) ;
    if( found == false )
    {
	DODSContainerPersistenceException pe;
	pe.set_error_description( "MySQL password not specified for " + n ) ;
	throw pe;
    }

    string my_database=TheDODSKeys->get_key( my_key + "database", found ) ;
    if( found == false )
    {
	DODSContainerPersistenceException pe;
	pe.set_error_description( "MySQL database not specified for " + n ) ;
	throw pe;
    }

    try
    {
	_query = new DODSMySQLQuery( my_server, my_user,
				     my_password, my_database ) ;
    }
    catch( bad_alloc::bad_alloc )
    {
	DODSMemoryException ex;
	ex.set_error_description("Can not get memory for Persistence object");
	ex.set_amount_of_memory_required(sizeof(DODSMySQLQuery));
	throw ex;
    }
}

DODSContainerPersistenceMySQL::~DODSContainerPersistenceMySQL()
{
    if( _query ) delete _query ;
    _query =0 ;
}

/** @brief looks for the specified container information in the MySQL
 * database.
 *
 * Using the symbolic name specified in the passed DODSContianer object, looks
 * up the container information for the symbolic name in the MySQL database
 * opened in the constructor.
 *
 * If a match is made with the symbolic name found in the container then the
 * information is stored in the passed container object and the is_valid flag
 * is set to true. If not found, then is_valid is set to false.
 *
 * @param d container to look for and, if found, store the information in.
 * @throws DODSContainerPersistenceException if the information in the
 * database is corrupt
 * @throws DODSMySQLQueryException if error running the query
 * @see DODSContainer
 */
void
DODSContainerPersistenceMySQL::look_for( DODSContainer &d )
{
    d.set_valid_flag( false ) ;
    string query = "select REAL_NAME, CONTAINER_TYPE from tbl_containers where SYMBOLIC_NAME=\"";
    query += d.get_symbolic_name().c_str();
    query += "\";";
    _query->run_query( query );
    if( !_query->is_empty_set() )
    {
	if( (_query->get_nrows() != 1) || (_query->get_nfields() != 2) )
	{
	    DODSContainerPersistenceException pe ;
	    pe.set_error_description( "Invalid data from MySQL" ) ;
	    throw pe ;
	}
	else
	{
	    d.set_valid_flag( true ) ;
	    _query->first_row() ;
	    _query->first_field() ;
	    d.set_real_name( _query->get_field() ) ;
	    _query->next_field() ;
	    d.set_container_type( _query->get_field() ) ;
	}
    }
}

/** @brief adds a container with the provided information
 *
 * This method adds a container to the persistence store with the
 * specified information. This functionality is not currently supported for
 * MySQL persistence.
 *
 * @param s_name symbolic name for the container
 * @param r_name real name for the container
 * @param type type of data represented by this container
 */
void
DODSContainerPersistenceMySQL::add_container( string s_name,
                                            string r_name,
					    string type )
{
    throw DODSContainerPersistenceException( "Unable to add a container to MySQL container persistence, not yet implemented\n" ) ;
}

/** @brief removes a container with the given symbolic name, not implemented
 * in this implementation class.
 *
 * This method removes a container to the persistence store with the
 * given symbolic name. It deletes the container.
 *
 * @param s_name symbolic name for the container
 * @return true if successfully removed and false otherwise
 */
bool
DODSContainerPersistenceMySQL::rem_container( const string &s_name )
{
    throw DODSContainerPersistenceException( "Unable to remove a container from a MySQL container persistece, not yet implemented\n" ) ;
    return false ;
}

/** @brief show information for each container in this persistent store
 *
 * For each container in this persistent store, add infomation about each of
 * those containers. The information added to the information object
 * includes a line for each container within this persistent store which 
 * includes the symbolic name, the real name, and the data type, 
 * separated by commas.
 *
 * In the case of this persistent store the MySQL database table is queried
 * and the symbolic name, real name and container type are retrieved and
 * added to the information object.
 *
 * @param info object to store the container and persistent store information
 * @see DODSInfo
 */
void
DODSContainerPersistenceMySQL::show_containers( DODSInfo &info )
{
    info.add_data( get_name() ) ;
    info.add_data( "\n" ) ;
    string query = "select SYMBOLIC_NAME, REAL_NAME, CONTAINER_TYPE from tbl_containers;" ;
    _query->run_query( query );
    if( !_query->is_empty_set() )
    {
	bool row = _query->first_row() ;
	while( row )
	{
	    if( ( _query->get_nfields() != 3 ) )
	    {
		DODSContainerPersistenceException pe ;
		pe.set_error_description( "Invalid data from MySQL" ) ;
		throw pe ;
	    }
	    _query->first_field() ;
	    string sym = _query->get_field() ;
	    _query->next_field() ;
	    string real = _query->get_field() ;
	    _query->next_field() ;
	    string type = _query->get_field() ;
	    string line = sym + "," + real + "," + type + "\n" ;
	    info.add_data( line ) ;
	}
    }
}

// $Log: DODSContainerPersistenceMySQL.cc,v $
// Revision 1.8  2005/03/17 20:37:50  pwest
// added documentation for rem_container and show_containers
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
