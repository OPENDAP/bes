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

#include "BESObj.h"
#include "BESDataHandlerInterface.h"

//class BESResponseHandler;
class BESXMLCommand;

// p_xmlcmd_builder: a pointer to a function that takes a BESDataHandlerInterface
// instance and returns a BESXMLCommand instance.
typedef BESXMLCommand *(*p_xmlcmd_builder)(const BESDataHandlerInterface &dhi);

class BESXMLCommand: public BESObj {
private:
    /// Bind names top_xmlcmn_builder functions; used by find_command(), et al.
    static std::map<std::string, p_xmlcmd_builder> cmd_list;
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
     * @brief Has a response handler been created given the request
     * document?
     *
     * @return true if a response handler has been set, false otherwise
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

    virtual void dump(ostream &strm) const;

    static void add_command(const std::string &cmd_str, p_xmlcmd_builder cmd);
    static bool del_command(const std::string &cmd_str);
    static p_xmlcmd_builder find_command(const std::string &cmd_str);
};

#endif // A_BESXMLCommand_h

