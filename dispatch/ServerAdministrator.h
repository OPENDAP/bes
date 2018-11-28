// ServerAdministrator.h
// -*- mode: c++; c-basic-offset:4 -*-
//
// This file is part of BES cmr_module
//
// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#ifndef _ServerAdministrator_h_
#define _ServerAdministrator_h_ 1

#include <list>
#include <string>
#include <map>

#define SERVER_ADMINISTRATOR_KEY "BES.ServerAdministrator"

namespace bes {

/**
 * @brief A ServerAdministrator object from the TheBESKeys associated with the string SERVER_ADMIN_KEY
 */
class ServerAdministrator {
private:
    std::map<std::string,std::string> d_admin_info;

public:
    ServerAdministrator();
    virtual ~ServerAdministrator(){}

    virtual std::string get(const string &key);

    virtual std::string xdump() const;
    virtual std::string jdump(bool compact=true) const;


};
} // namespace bes

#endif // _ServerAdministrator_h_

