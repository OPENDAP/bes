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
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef BESException_h_
#define BESException_h_ 1

#include <string>

using std::string ;

#include "BESObj.h"
#include "BESStatusReturn.h"

/** @brief Abstract exception class for the BES with basic string message
 *
 */
class BESException : public BESObj
{
protected:
    string		_msg ;
    string		_context ;
    int			_return_code ;
    string		_file ;
    int			_line ;

    			BESException() { _msg = "UNDEFINED" ; }
public:
    			BESException( const string &msg,
			              const string &file,
				      int line )
			    : _msg( msg ),
			      _return_code( 0 ),
			      _file( file ),
			      _line( line ) {}
    virtual		~BESException() {}

    /** @brief set the error message for this exception
     *
     * @param msg message string
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

    /** @brief Set the context name of the error class
     *
     * Gives the error context, such as response or request or transmit
     * @param context name of the context
     */
    virtual void	set_context( const string &context )
			{
			    _context = context ;
			}

    /** @brief Return the context name of the error class
     *
     * Gives the error context, such as response or request or transmit
     * @return context string
     */
    virtual string	get_context() { return _context ; }

    /** @brief Set the return code for this particular error class
     *
     * Sets the return code for this error class, which could represent the
     * need to terminate or do something specific based on the error.
     * @param return_code code used when returning from the error
     */
    virtual void	set_return_code( int return_code )
			{
			    _return_code = return_code ;
			}

    /** @brief Return the return code for this error class
     *
     * Returns the return code for this error class, which could represent
     * the need to terminate or do something specific base on the error
     * @return context string
     */
    virtual int		get_return_code() { return _return_code ; }

    /** @brief Displays debug information about this object
     *
     * @param strm output stream to use to dump the contents of this object
     */
    virtual void	dump( ostream &strm ) const ;
};

#endif // BESException_h_ 

