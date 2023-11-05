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
    std::unordered_map<std::string,std::string> d_admin_info;
    std::string d_organization;
    std::string d_street;
    std::string d_city;
    std::string d_region;
    std::string d_postal_code;
    std::string d_country;
    std::string d_telephone;
    std::string d_email;
    std::string d_website;

    /**
    BES.ServerAdministrator=email:support@opendap.org
    BES.ServerAdministrator+=organization:OPeNDAP Inc.
    BES.ServerAdministrator+=street:165 NW Dean Knauss Dr.
    BES.ServerAdministrator+=city:Narragansett
    BES.ServerAdministrator+=region:RI
    BES.ServerAdministrator+=postalCode:02882
    BES.ServerAdministrator+=country:US
    BES.ServerAdministrator+=telephone:+1.401.575.4835
    BES.ServerAdministrator+=website:http://www.opendap.org
    **/

public:
    ServerAdministrator();
    virtual ~ServerAdministrator(){}

    virtual std::string get(const std::string &key);

    virtual void mk_default();

    virtual std::string xdump() const;
    virtual std::string jdump(bool compact=true) const;

    virtual std::string get_organization() const { return  d_organization; }
    virtual std::string get_street() const { return  d_street; }
    virtual std::string get_city() const { return  d_city; }
    virtual std::string get_region() const { return  d_region; }
    virtual std::string get_state() const { return  d_region; }
    virtual std::string get_postal_code() const { return  d_postal_code; }
    virtual std::string get_country() const { return  d_country; }
    virtual std::string get_telephone() const { return  d_telephone; }
    virtual std::string get_email() const { return  d_email; }
    virtual std::string get_website() const { return  d_website; }


}; //class ServerAdministrator

} // namespace bes

#endif // _ServerAdministrator_h_

