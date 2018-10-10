// BESError.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef BESError_h_
#define BESError_h_ 1

#include <string>

#include "BESObj.h"

#define BES_INTERNAL_ERROR 1

// BES_INTERNAL_FATAL_ERROR will cause the bes listener to exit()
// while the others (BES_INTERNAL_ERROR, ...) won't.
// But, see the new option BES.ExitOnInternalError, which causes the
// BES to exit on an InternalError.
#define BES_INTERNAL_FATAL_ERROR 2

#define BES_SYNTAX_USER_ERROR 3
#define BES_FORBIDDEN_ERROR 4
#define BES_NOT_FOUND_ERROR 5

// I added this for the timeout feature. jhrg 12/28/15
#define BES_TIMEOUT_ERROR 6

/** @brief Abstract exception class for the BES with basic string message
 *
 */
class BESError: public BESObj {
protected:
    std::string _msg;
    unsigned int _type;
    std::string _file;
    unsigned int _line;

    BESError(): _msg("UNDEFINED"), _type(0), _file(""), _line(0) { }

public:
    /** @brief constructor that takes message, type of error, source file
     * the error originated and the line number in the source file
     *
     * @param msg error message
     * @param type type of error generated. Default list of error types are
     * defined above as internal error, internal fatal error, syntax/user
     * error, resource forbidden error, resource not found error.
     * @param file the filename in which this error object was created
     * @param line the line number within the file in which this error
     * object was created
     */
    BESError(const std::string &msg, unsigned int type, const std::string &file, unsigned int line) :
            _msg(msg), _type(type), _file(file), _line(line)
    {
    }
    virtual ~BESError()
    {
    }

    /** @brief set the error message for this exception
     *
     * @param msg message string
     */
    virtual void set_message(const std::string &msg)
    {
        _msg = msg;
    }
    /** @brief get the error message for this exception
     *
     * @return error message
     */
    virtual std::string get_message()
    {
        return _msg;
    }
    /** @brief get the file name where the exception was thrown
     *
     * @return file name
     */
    virtual std::string get_file()
    {
        return _file;
    }
    /** @brief get the line number where the exception was thrown
     *
     * @return line number
     */
    virtual int get_line()
    {
        return _line;
    }

    // Return the message, file and line
    virtual std::string get_verbose_message();

    /** @brief Set the return code for this particular error class
     *
     * Sets the return code for this error class, which could represent the
     * need to terminate or do something specific based on the error.
     *
     * @param type the type of error this error object represents. Can
     * be one of BES_INTERNAL_ERROR, BES_INTERNAL_FATAL_ERROR,
     * BES_SYNTAX_USER_ERROR, BES_FORBIDDEN_ERROR, BES_NOT_FOUND_ERROR
     */
    virtual void set_bes_error_type(int type)
    {
        _type = type;
    }

    /** @brief Return the return code for this error class
     *
     * Returns the return code for this error class, which could represent
     * the need to terminate or do something specific base on the error
     * @return context string
     */
    virtual int get_bes_error_type()
    {
        return _type;
    }

    /** @brief Displays debug information about this object
     *
     * @param strm output stream to use to dump the contents of this object
     */
    virtual void dump(std::ostream &strm) const;
};

#endif // BESError_h_ 
