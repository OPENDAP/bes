// BESXMLCommand.h

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

#ifndef A_BESXMLCommand_h
#define A_BESXMLCommand_h 1

#include <string>
#include <map>

#include <libxml/encoding.h>
#include <libxml/tree.h>

#include "BESObj.h"
#include "BESDataHandlerInterface.h"

//class BESResponseHandler;
class BESXMLCommand;

/**
 * A pointer to a function that takes a BESDataHandlerInterface instance (passed
 * by reference) and returns a pointer to a new BESXMLCommand instance.
 */
typedef BESXMLCommand *(*p_xmlcmd_builder)(const BESDataHandlerInterface &dhi);

/**
 * @brief Base class for the BES's commands
 *
 * Maintains a factory for used to build all the BES's commands. This class
 * also holds instances of the DataHandlerInterface used by a particular instance
 * of a command and the information about this command that should be written
 * to the BES log.
 *
 * @ The set_response() is used to form a linkage between the code
 */
class BESXMLCommand: public BESObj {
private:
    /// Bind names top_xmlcmn_builder functions; used by find_command(), et al.
    static std::map<std::string, p_xmlcmd_builder> factory;
    typedef std::map<std::string, p_xmlcmd_builder>::iterator cmd_iter;

protected:
    BESDataHandlerInterface d_xmlcmd_dhi;
    virtual void set_response();

    /// Used only for the log.
    std::string d_cmd_log_info;

    BESXMLCommand(const BESDataHandlerInterface &base_dhi);

public:
    virtual ~BESXMLCommand()
    {
    }

    /**
     * @brief Parse the XML request document beginning at the given node
     *
     * @param node Begin parsing at this XML node
     */
    virtual void parse_request(xmlNode *node) = 0;

    /**
     * @brief Does this command return a response to the client?
     *
     *  Every command has an associated ResponseHandler, but not all ResponseHandlers
     *  return information to the BES's client. In fact, for any group of commands sent
     *  to the BES, only *one* can return information (except for errors, which stop
     *  command processing). If this command does not normally return a response (text
     *  or binary data), the value of this method should be false.
     *
     * @return true if it returns a response, false otherwise
     */
    virtual bool has_response() = 0;

    /**
     * @brief Prepare any information needed to execute the request of
     * this command
     */
    virtual void prep_request()
    {
    }

    /**
     * @brief Return the current BESDataHandlerInterface
     *
     * Since there can be multiple commands within a single request
     * document, different interface objects can be created. This
     * returns the current interface object
     *
     * @return The current BESDataHandlerInterface object
     */
    virtual BESDataHandlerInterface &get_xmlcmd_dhi()
    {
        return d_xmlcmd_dhi;
    }

    void dump(std::ostream &strm) const override;

    static void add_command(const std::string &cmd_str, p_xmlcmd_builder cmd);
    static void del_command(const std::string &cmd_str);
    static p_xmlcmd_builder find_command(const std::string &cmd_str);
};

#endif // A_BESXMLCommand_h

