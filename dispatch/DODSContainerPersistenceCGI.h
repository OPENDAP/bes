// DODSContainerPersistenceCGI.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

#ifndef DODSContainerPersistenceCGI_h_
#define DODSContainerPersistenceCGI_h_ 1

#include <map>
#include <string>

using std::map ;
using std::string ;

#include "DODSContainerPersistence.h"

/** @brief implementation of DODSContainerPersistence that represents a
 * regular expression means of determining a data type.
 *
 * This implementation of DODSContainerPersistence looks the name of a data
 * file as the symbolic name of the container and compares that data file to a
 * set of regular expressions to determine the type of data it is. The
 * implementation gets these regular expressions from the dods initialization
 * file using TheDODSKeys as well as the base directory for where the files
 * exist.
 *
 * DODS.Container.Persistence.CGI.&lt;name&gt;.BaseDirectory is the key
 * representing the base directory where the files are physically located.
 * The real_name of the container is determined by concatenating the file
 * name to the base directory.
 *
 * DODS.Container.Persistence.CGI.&lt;name&gt;.TypeMatch is the key
 * representing the regular expressions. This key is formatted as follows:
 *
 * &lt;data type&gt;:&lt;reg exp&gt;;&lt;data type&gt;:&lt;reg exp&gt;;
 *
 * For example: cedar:cedar\/.*\.cbf;cdf:cdf\/.*\.cdf;
 *
 * The first would match anything that might look like: cedar/datfile01.cbf
 *
 * &lt;name&gt; is the name of this persistent store, so you could have
 * multiple persistent stores using regular expressions.
 *
 * @see DODSContainerPersistence
 * @see DODSContainer
 * @see DODSKeys
 */
class DODSContainerPersistenceCGI : public DODSContainerPersistence
{
private:
    map< string, string > _match_list ;
    typedef map< string, string >::const_iterator Match_list_citer ;

    string			_base_dir ;

public:
    				DODSContainerPersistenceCGI( const string &n ) ;
    virtual			~DODSContainerPersistenceCGI() ;

    virtual void		look_for( DODSContainer &d ) ;
    virtual void		add_container( string s_name, string r_name,
					       string type ) ;
    virtual bool		rem_container( const string &s_name ) ;

    virtual void		show_containers( DODSInfo &info ) ;
};

#endif // DODSContainerPersistenceCGI_h_

// $Log: DODSContainerPersistenceCGI.h,v $
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
