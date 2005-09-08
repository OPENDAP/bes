// DODSContainerPersistenceCGI.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSContainerPersistenceCGI.h"
#include "DODSContainer.h"
#include "DODSContainerPersistenceException.h"
#include "TheDODSKeys.h"
#include "GNURegex.h"
#include "DODSInfo.h"

/** @brief create an instance of this persistent store with the given name.
 *
 * Creates an instances of DODSContainerPersistenceCGI with the given name.
 * Looks up the base directory and regular expressions in the dods
 * initialization file using TheDODSKeys. THrows an exception if either of
 * these cannot be determined or if the regular expressions are incorrectly
 * formed.
 *
 * &lt;data type&gt;:&lt;reg exp&gt;;&lt;data type&gt;:&lt;reg exp&gt;;
 *
 * each type/reg expression pair is separated by a semicolon and ends with a
 * semicolon. The data type/expression pair itself is separated by a
 * semicolon.
 *
 * @param n name of this persistent store
 * @throws DODSContainerPersistenceException if unable to find the base
 * directory or regular expressions in the dods initialization file. Also
 * thrown if the type matching expressions are malformed.
 * @see DODSKeys
 * @see DODSContainer
 */
DODSContainerPersistenceCGI::DODSContainerPersistenceCGI( const string &n )
    : DODSContainerPersistence( n )
{
    string base_key = "DODS.Container.Persistence.CGI." + n + ".BaseDirectory" ;
    bool found = false ;
    _base_dir = TheDODSKeys->get_key( base_key, found ) ;
    if( _base_dir == "" )
    {
	string s = base_key + " not defined in key file" ;
	DODSContainerPersistenceException pe ;
	pe.set_error_description( s ) ;
	throw pe;
    }

    string key = "DODS.Container.Persistence.CGI." + n + ".TypeMatch" ;
    string curr_str = TheDODSKeys->get_key( key, found ) ;
    if( curr_str == "" )
    {
	string s = key + " not defined in key file" ;
	DODSContainerPersistenceException pe ;
	pe.set_error_description( s ) ;
	throw pe;
    }

    int str_begin = 0 ;
    int str_end = curr_str.length() ;
    int semi = 0 ;
    bool done = false ;
    while( done == false )
    {
	semi = curr_str.find( ";", str_begin ) ;
	if( semi == -1 )
	{
	    string s = (string)"CGI type match malformed, no semicolon, "
		       "looking for type:regexp;[type:regexp;]" ;
	    DODSContainerPersistenceException pe ;
	    pe.set_error_description( s ) ;
	    throw pe;
	}
	else
	{
	    string a_pair = curr_str.substr( str_begin, semi-str_begin ) ;
	    str_begin = semi+1 ;
	    if( semi == str_end-1 )
	    {
		done = true ;
	    }

	    int col = a_pair.find( ":" ) ;
	    if( col == -1 )
	    {
		string s = (string)"CGI type match malformed, no colon, "
			   + "looking for type:regexp;[type:regexp;]" ;
		DODSContainerPersistenceException pe ;
		pe.set_error_description( s ) ;
		throw pe;
	    }
	    else
	    {
		string name = a_pair.substr( 0, col ) ;
		string val = a_pair.substr( col+1, a_pair.length()-col ) ;
		_match_list[name] = val ;
	    }
	}
    }
}

DODSContainerPersistenceCGI::~DODSContainerPersistenceCGI()
{ 
}

/** @brief looks for the specified container using the regular expressiono
 * matching.
 *
 * If a match is made with the symbolic name found in the container then the
 * information is stored in the passed container object and the is_valid flag
 * is set to true. If not found, then is_valid is set to false.
 *
 * The real name of the container (the file name) is constructed using the
 * base directory from the dods initialization file with the symbolic name
 * appended to it.
 *
 * @param d container to look for and, if found, store the information in.
 */
void
DODSContainerPersistenceCGI::look_for( DODSContainer &d )
{
    d.set_valid_flag( false ) ;
    string sym_name = d.get_symbolic_name() ;
    DODSContainerPersistenceCGI::Match_list_citer i = _match_list.begin() ;
    DODSContainerPersistenceCGI::Match_list_citer ie = _match_list.end() ;
    for( ; i != ie; i++ )
    {
	string reg = (*i).second ;
	Regex reg_expr( reg.c_str() ) ;
	if( reg_expr.match( sym_name.c_str(), sym_name.length() ) != -1 )
	{
	    d.set_container_type( (*i).first ) ;
	    string real_name = _base_dir + "/" + d.get_symbolic_name() ;
	    d.set_real_name( real_name ) ;
	    d.set_valid_flag( true ) ;
	    break ;
	}
    }
}

/** @brief adds a container with the provided information
 *
 * This method adds a container to the persistence store with the
 * specified information. This functionality is not currently supported for
 * cgi persistence.
 *
 * @param s_name symbolic name for the container
 * @param r_name real name for the container
 * @param type type of data represented by this container
 */
void
DODSContainerPersistenceCGI::add_container( string ,
                                            string ,
					    string )
{
    throw DODSContainerPersistenceException( "Unable to add a container to CGI persistence\n" ) ;
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
DODSContainerPersistenceCGI::rem_container( const string & )
{
    throw DODSContainerPersistenceException( "Unable to remove a container from CGI persistence\n" ) ;
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
 * In the case of this persistent store the symbolic name is the regular
 * expression and the real name is the base directory for that regular
 * expression followed by the regular expression and the type of data that
 * that regular expression matches.
 *
 * @param info object to store the container and persistent store information into
 * @see DODSInfo
 */
void
DODSContainerPersistenceCGI::show_containers( DODSInfo &info )
{
    info.add_data( get_name() ) ;
    info.add_data( "\n" ) ;
    DODSContainerPersistenceCGI::Match_list_citer i = _match_list.begin() ;
    DODSContainerPersistenceCGI::Match_list_citer ie = _match_list.end() ;
    for( ; i != ie; i++ )
    {
	string reg = (*i).second ;
	string type = (*i).first ;
	string real = _base_dir + "/" + reg ;
	string line = reg + "," + real + "," + type + "\n" ;
	info.add_data( line ) ;
    }
}

// $Log: DODSContainerPersistenceCGI.cc,v $
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
