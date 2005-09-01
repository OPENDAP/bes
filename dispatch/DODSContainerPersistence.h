// DODSContainerPersistence.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSContainerPersistence_h_
#define DODSContainerPersistence_h_ 1

#include <string>

using std::string ;

class DODSContainer ;
class DODSInfo ;

/** @brief provides persistent storage for data storage information
 * represented by a symbolic name.
 *
 * An implementation of the abstract interface DODSContainerPersistence
 * provides storage for information about accessing data of different types.
 * The information is represented by a symbolic name. A user can request a
 * symbolic name that represents a certain data file of a certain data type.
 * For example, a symbolic name 'nc1' could represent the netcdf file
 * /usr/apache/htdocs/netcdf/datfile01.cdf.
 *
 * An instance of a derived implementation has a name associated with it, in
 * case that there are multiple ways in which the information can be stored.
 * For example, the main persistent storage for containers could be a mysql
 * database, but a user could store temporary information in different files.
 * If the user wishes to remove one of these persistence stores they would
 * request that a named DODSContainerPersistence object be removed from the
 * list.
 * 
 * @see DODSContainer
 * @see DODSContainerPersistenceList
 */
class DODSContainerPersistence
{
protected:
    string		_my_name ;

public:
    /** @brief create an instance of DODSContainerPersistence with the give
     * name.
     *
     * @param name name of this persistence store
     */
    				DODSContainerPersistence( const string &name )
				    : _my_name( name ) {} ;

    virtual 			~DODSContainerPersistence() {} ;

    /** @brief retrieve the name of this persistent store
     *
     * @return name of this persistent store.
     */
    virtual string		get_name() { return _my_name ; }

    /** @brief looks for a container in this persistent store
     *
     * This method looks for a container with the name specified in the given
     * container and fills in the information in the given container.
     *
     * @param d The container to look for and, if found, the information is
     * filled in.
     */
    virtual void 		look_for( DODSContainer &d ) = 0 ;

    /** @brief adds a container with the provided information
     *
     * This method adds a container to the persistence store with the
     * specified information.
     *
     * @param s_name symbolic name for the container
     * @param r_name real name for the container
     * @param type type of data represented by this container
     */
    virtual void		add_container( string s_name, string r_name,
					       string type ) = 0 ;

    /** @brief removes a container with the given symbolic name
     *
     * This method removes a container to the persistence store with the
     * given symbolic name. It deletes the container.
     *
     * @param s_name symbolic name for the container
     * @return true if successfully removed and false otherwise
     */
    virtual bool		rem_container( const string &s_name ) = 0 ;

    /** @brief show the containers stored in this persistent store
     *
     * Add information to the passed information object about each of the
     * containers stored within this persistent store. The information
     * added to the passed information objects includes the name of this
     * persistent store on the first line followed by the symbolic name,
     * real name and data type for each container, one per line.
     *
     * @param info information object to store the information in
     */
    virtual void		show_containers( DODSInfo &info ) = 0 ;
};

#endif // DODSContainerPersistence_h_

// $Log: DODSContainerPersistence.h,v $
// Revision 1.9  2005/03/17 20:37:50  pwest
// added documentation for rem_container and show_containers
//
// Revision 1.8  2005/03/17 19:23:58  pwest
// deleting the container in rem_container instead of returning the removed container, returning true if successfully removed and false otherwise
//
// Revision 1.7  2005/03/15 19:55:36  pwest
// show containers and show definitions
//
// Revision 1.6  2005/02/02 00:03:13  pwest
// ability to replace containers and definitions
//
// Revision 1.5  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.4  2004/12/15 17:36:01  pwest
//
// Changed the way in which the parser retrieves container information, going
// instead to ThePersistenceList, which goes through the list of container
// persistence instances it has.
//
// Revision 1.3  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.2  2004/07/09 16:10:29  pwest
// Removed static var in DODSContainerPersistence to check if strict or nice
// had been already set. In DODSInfo only using one key to see if information
// buffered or not unless a different key is passed in from child class.
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
