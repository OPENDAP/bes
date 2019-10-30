// BESServerHandler.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2011 OPeNDAP
// Author: James Gallagher <jgallagher@opendap.org> Based on code by Partick
// West and Jose Garcia.
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
 
// (c) COPYRIGHT OPeNDAP
// Please read the full copyright statement in the file COPYING.

#ifndef DaemonCommandHandler_h
#define DaemonCommandHandler_h 1

#include <string>
#include <vector>

#include "ServerHandler.h"
#include "BESXMLWriter.h"

class Connection ;

class DaemonCommandHandler: public ServerHandler {
private:
    typedef enum {
        HAI_UNKNOWN,
        HAI_STOP_NOW,
        HAI_START,
        HAI_EXIT,
        HAI_GET_CONFIG,
        HAI_SET_CONFIG,
        HAI_TAIL_LOG,
        HAI_GET_LOG_CONTEXTS,
        HAI_SET_LOG_CONTEXT
    } hai_command;

    string d_bes_conf;
    string d_config_dir;
    string d_include_dir;

    // Build a map of all the various config files. This map relates the name
    // of the config file (eg 'bes.conf') to the full pathname for that file.
    // Only the name of config file is shown in responses; we use the map to
    // actually find/read/write the file.
    std::map<string,string> d_pathnames;

    string d_log_file_name;

    void load_include_files(std::vector<string> &files, const string &keys_file_name);
    void load_include_file(const string &files, const string &keys_file_name);

    hai_command lookup_command(const string &command);
    void execute_command(const string &command, BESXMLWriter &writer);

public:
    DaemonCommandHandler(const string &config);
    virtual ~DaemonCommandHandler() { }

    string get_config_file() { return d_bes_conf; }
    void set_config_file(const string &config) { d_bes_conf = config; }

    virtual void handle(Connection *c);

    virtual void dump(std::ostream &strm) const;
};

#endif // DaemonCommandHandler_h

