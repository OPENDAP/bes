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
#include <stdexcept>

#include "BESObj.h"

// Forward declaration of BESInfo since BESInfo.h
// already includes BESError.h
class BESInfo;

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

// A BES_HTTP_ERROR is thrown when a request to another service fails.
#define BES_HTTP_ERROR 7

/**
 * @brief Base exception class for the BES with basic string message
 */
class BESError: public std::exception,  public BESObj {
private:
    std::string _msg {"UNDEFINED"};
    unsigned int _type {0};
    std::string _file;
    unsigned int _line {0};


public:
    BESError() = default;

    /** @brief constructor that takes message, type of error, source file
     * the error originated and the line number in the source file
     *
     * @param msg error message. This is the information returned by what().
     * @param type type of error generated. Default list of error types are
     * defined above as internal error, internal fatal error, syntax/user
     * error, resource forbidden error, resource not found error.
     * @param file the filename in which this error object was created
     * @param line the line number within the file in which this error
     * object was created
     */
    BESError(std::string msg, unsigned int type, std::string file, unsigned int line) :
            _msg(std::move(msg)), _type(type), _file(std::move(file)), _line(line)
    { }
    
    /**
     * @note Define this copy constructor as noexcept. See the web for why (e.g.,
     * https://stackoverflow.com/questions/28627348/noexcept-and-copy-move-constructors)
     */
    BESError(const BESError &src) noexcept
        : exception(), _msg(src._msg), _type(src._type), _file(src._file), _line(src._line) { }

    ~BESError() override = default;

    /**
     * @note Follow the rule of three - see https://en.cppreference.com/w/cpp/language/rule_of_three
     */
    BESError &operator=(const BESError &rhs) = delete;

    /** @brief set the error message for this exception
     *
     * @param msg message string
     */
    void set_message(const std::string &msg)
    {
        _msg = msg;
    }

    /**
     * Used to add error specific details to the BESInfo object
     * @param info
     */
    virtual void add_my_error_details_to(BESInfo &info) const {
        // Some errors are covered by the basic details and the message.
        // Others, like HttpError require more information to be carried
        // in the error so the override this method.
    }

    /** @brief get the error message for this exception
     *
     * @return error message
     */
    std::string get_message() const
    {
        return _msg;
    }

    /** @brief get the file name where the exception was thrown
     *
     * @return file name
     */
    std::string get_file() const
    {
        return _file;
    }

    /** @brief get the line number where the exception was thrown
     *
     * @return line number
     */
    unsigned int get_line() const
    {
        return _line;
    }

    // Return the message, file and line. Over load this for special messages, etc.
    virtual std::string get_verbose_message() const;

    /** @brief Set the return code for this particular error class
     *
     * Sets the return code for this error class, which could represent the
     * need to terminate or do something specific based on the error.
     *
     * @param type the type of error this error object represents. Can
     * be one of BES_INTERNAL_ERROR, BES_INTERNAL_FATAL_ERROR,
     * BES_SYNTAX_USER_ERROR, BES_FORBIDDEN_ERROR, BES_NOT_FOUND_ERROR
     */
    void set_bes_error_type(unsigned int type)
    {
        _type = type;
    }

    /** @brief Return the return code for this error class
     *
     * Returns the return code for this error class, which could represent
     * the need to terminate or do something specific base on the error
     * @return context string
     */
    unsigned int get_bes_error_type() const
    {
        return _type;
    }

    // The pointer is valid only for the lifetime of the BESError instance. jhrg 3/29/22

    /**
     * @brief Return a brief message about the exception
     * @return A char* that points to the message and is valid for the lifetime of this instance.
     */
    const char* what() const noexcept override {
        return _msg.c_str();
    }

    /** @brief Displays debug information about this object
     *
     * @param strm output stream to use to dump the contents of this object
     */
    void dump(std::ostream &strm) const override;
};

#endif // BESError_h_ 
