// DODSContainerPersistenceFile.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <sstream>
#include <fstream>
#include <iostream>

using std::stringstream ;
using std::ifstream ;

#include "DODSContainerPersistenceFile.h"
#include "DODSContainer.h"
#include "TheDODSKeys.h"
#include "DODSContainerPersistenceException.h"
#include "DODSInfo.h"

/** @brief pull container information from the specified file
 *
 * Constructs a DODSContainerPersistenceFile from a file specified by
 * a key in the dods initialization file. The key is constructed using the
 * name of this persistent store.
 *
 * DODS.Container.Persistence.File.<name>
 *
 * where <name> is the name of this persistent store.
 *
 * The containers are then read into memory. The format of the file is as
 * follows.
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
 * @param n name of this persistent store
 * @throws DODSContainerPersistenceException if the file can not be opened or
 * if there is an error in reading in the container information.
 * @see DODSContainerPersistence
 * @see DODSContainer
 * @see DODSContainerPersistenceException
 */
DODSContainerPersistenceFile::DODSContainerPersistenceFile( const string &n )
    : DODSContainerPersistence( n )
{
    string key = "DODS.Container.Persistence.File." + n ;
    bool found = false ;
    string my_file = TheDODSKeys->get_key( key, found ) ;
    if( my_file == "" )
    {
	string s = key + " not defined in key file" ;
	DODSContainerPersistenceException pe ;
	pe.set_error_description( s ) ;
	throw pe;
    }

    ifstream persistence_file( my_file.c_str() ) ;
    if( !persistence_file )
    {
	string s = "Unable to open persistence file " + my_file ;
	DODSContainerPersistenceException pe ;
	pe.set_error_description( s ) ;
	throw pe;
    }

    char cline[80] ;

    while( !persistence_file.eof() )
    {
	stringstream strm ;
	persistence_file.getline( cline, 80 ) ;
	if( !persistence_file.eof() )
	{
	    strm << cline ;
	    DODSContainerPersistenceFile::container *c =
		new DODSContainerPersistenceFile::container ;
	    strm >> c->_symbolic_name ;
	    strm >> c->_real_name ;
	    strm >> c->_container_type ;
	    string dummy ;
	    strm >> dummy ;
	    if( c->_symbolic_name == "" ||
		c->_real_name == "" ||
		c->_container_type == "" )
	    {
		delete c ;
		string s = "Incomplete container persistence line in file "
			   + my_file ;
		DODSContainerPersistenceException pe ;
		pe.set_error_description( s ) ;
		throw pe;
	    }
	    if( dummy != "" )
	    {
		delete c ;
		string s = "Too many fields in persistence file "
			   + my_file ;
		DODSContainerPersistenceException pe ;
		pe.set_error_description( s ) ;
		throw pe;
	    }
	    _container_list[c->_symbolic_name] = c ;
	}
    }
    persistence_file.close() ;
}

DODSContainerPersistenceFile::~DODSContainerPersistenceFile()
{
    DODSContainerPersistenceFile::Container_citer i = _container_list.begin() ;
    DODSContainerPersistenceFile::Container_citer ie = _container_list.end() ;
    for( ; i != ie; i++ )
    {
	DODSContainerPersistenceFile::container *c = (*i).second ;
	delete c ;
    }
}

/** @brief looks for the specified container in the list of containers loaded
 * from the file.
 *
 * If a match is made with the symbolic name found in the container then the
 * information is stored in the passed container object and the is_valid flag
 * is set to true. If not found, then is_valid is set to false.
 *
 * @param d container to look for and, if found, store the information in.
 * @see DODSContainer
 */
void
DODSContainerPersistenceFile::look_for( DODSContainer &d )
{
    d.set_valid_flag( false ) ;
    DODSContainerPersistenceFile::Container_citer i ;
    i = _container_list.find( d.get_symbolic_name() ) ;
    if( i != _container_list.end() )
    {
	DODSContainerPersistenceFile::container *c = (*i).second;
	d.set_real_name( c->_real_name ) ;
	d.set_container_type( c->_container_type ) ;
	d.set_valid_flag( true ) ;
    }
}

void
DODSContainerPersistenceFile::add_container( string s_name,
                                            string r_ame,
					    string type )
{
    throw DODSContainerPersistenceException( "Unable to add a container to a file, not yet implemented\n" ) ;
}

/** @brief removes a container with the given symbolic name
 *
 * This method removes a container to the persistence store with the
 * given symbolic name. It deletes the container. The container is NOT
 * removed fromt he file from which it was loaded, however.
 *
 * @param s_name symbolic name for the container
 * @return true if successfully removed and false otherwise
 */
bool
DODSContainerPersistenceFile::rem_container( const string &s_name )
{
    bool ret = false ;
    DODSContainerPersistenceFile::Container_iter i ;
    i = _container_list.find( s_name ) ;
    if( i != _container_list.end() )
    {
	DODSContainerPersistenceFile::container *c = (*i).second;
	_container_list.erase( i ) ;
	delete c ;
	ret = true ;
    }
    return ret ;
}

/** @brief show information for each container in this persistent store
 *
 * For each container in this persistent store, add infomation about each of
 * those containers. The information added to the information object
 * includes a line for each container within this persistent store which 
 * includes the symbolic name, the real name, and the data type, 
 * separated by commas.
 *
 * In the case of this persistent store all of the containers loaded from
 * the file specified by the key DODS.Container.Persistence.File.<store_name>
 * is added to the information object.
 *
 * @parameter info object to store the container and persistent store information into
 * @see DODSInfo
 */
void
DODSContainerPersistenceFile::show_containers( DODSInfo &info )
{
    info.add_data( get_name() ) ;
    info.add_data( "\n" ) ;
    DODSContainerPersistenceFile::Container_citer i ;
    i = _container_list.begin() ;
    for( i = _container_list.begin(); i != _container_list.end(); i++ )
    {
	DODSContainerPersistenceFile::container *c = (*i).second;
	string sym = c->_symbolic_name ;
	string real = c->_real_name ;
	string type = c->_container_type ;
	string line = sym + "," + real + "," + type + "\n" ;
	info.add_data( line ) ;
    }
}
// $Log: DODSContainerPersistenceFile.cc,v $
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
