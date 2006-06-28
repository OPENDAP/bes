// BESException.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef BESException_h_
#define BESException_h_ 1

#include <string>

using std::string ;

/** @brief Abstract exception class for OpenDAP with basic string message
 *
 */
class BESException
{
protected:
    string		_msg ;
    string		_file ;
    int			_line ;

    			BESException() { _msg = "UNDEFINED" ; }
public:
    			BESException( const string &msg,
			              const string &file,
				      int line )
			    : _msg( msg ),
			      _file( file ),
			      _line( line ) {}
    virtual		~BESException() {}

    /** @brief set the error message for this exception
     *
     * @param s message string
     */
    virtual void	set_message( const string &msg ) { _msg = msg ; }
    /** @brief get the error message for this exception
     *
     * @return error message
     */
    virtual string	get_message() { return _msg ; }
    /** @brief get the file name where the exception was thrown
     *
     * @return file name
     */
    virtual string	get_file() { return _file ; }
    /** @brief get the line number where the exception was thrown
     *
     * @return line number
     */
    virtual int		get_line() { return _line ; }
};

#endif // BESException_h_ 

