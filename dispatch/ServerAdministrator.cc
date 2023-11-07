
// ServerAdministrator.cc
// -*- mode: c++; c-basic-offset:4 -*-
//
//
// This file is part of BES httpd_catalog_module
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
#include "config.h"

#include <vector>
#include <map>
#include <sstream>

#include "TheBESKeys.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESLog.h"
#include "BESInternalFatalError.h"

#include "ServerAdministrator.h"

using std::vector;
using std::endl;
using std::string;
using std::ostream;

#define MODULE "bes"

#define prolog std::string("ServerAdministrator::").append(__func__).append("() - ")


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

#define EMAIL_KEY "email"
#define EMAIL_DEFAULT "support@opendap.org"

#define ORGANIZATION_KEY "organization"
#define ORGANIZATION_DEFAULT "OPeNDAP Inc."

#define STREET_KEY "street"
#define STREET_DEFAULT "165 NW Dean Knauss Dr."

#define CITY_KEY "city"
#define CITY_DEFAULT "Narragansett"

#define REGION_KEY     "region"
#define STATE_KEY      "state"
#define REGION_DEFAULT "RI"

#define POSTAL_CODE_KEY "postalCode"
#define POSTAL_CODE_DEFAULT "02882"

#define COUNTRY_KEY "country"
#define COUNTRY_DEFAULT "US"

#define TELEPHONE_KEY "telephone"
#define TELEPHONE_DEFAULT "+1.401.575.4835"

#define WEBSITE_KEY "website"
#define WEBSITE_DEFAULT "http://www.opendap.org"


namespace bes {

/**
 * Tries to read the SERVER_ADMINISTRATOR_KEY keys from the BESKeys if there is a prblem in the formatting
 * of the entries the default administrator (OPeNDAP Inc.) is returned.
 */
ServerAdministrator::ServerAdministrator(){
    bool found = false;


    TheBESKeys::TheKeys()->get_values(SERVER_ADMINISTRATOR_KEY,d_admin_info, true, found);


    bool bad_flag = false;

    d_organization = get(ORGANIZATION_KEY);
    if(d_organization.empty()){
        BESDEBUG(MODULE,__func__ << "() -  The configuration entry for the " <<
            SERVER_ADMINISTRATOR_KEY << "[" << ORGANIZATION_KEY << "] was missing." << endl);
        bad_flag = true;
    }

    d_street = get(STREET_KEY);
    if(d_street.empty()){
        BESDEBUG(MODULE,__func__ << "() -  The configuration entry for the " <<
            SERVER_ADMINISTRATOR_KEY << "[" << STREET_KEY << "] was missing." << endl);
        bad_flag = true;
    }

    d_city = get(CITY_KEY);
    if(d_city.empty()){
        BESDEBUG(MODULE,__func__ << "() -  The configuration entry for the " <<
            SERVER_ADMINISTRATOR_KEY << "[" << CITY_KEY << "] was missing." << endl);
        bad_flag = true;
    }

    d_region = get(REGION_KEY);
    if(d_region.empty()){
        BESDEBUG(MODULE,__func__ << "() -  The configuration entry for the " <<
            SERVER_ADMINISTRATOR_KEY << "[" << REGION_KEY << "] was missing." <<  endl);
        d_region = get(STATE_KEY);

        if(d_region.empty()){
            BESDEBUG(MODULE,__func__ << "() -  The configuration entry for the " <<
                SERVER_ADMINISTRATOR_KEY << "[" << STATE_KEY << "] was missing." << endl);
            bad_flag = true;
        }
    }

    d_postal_code = get(POSTAL_CODE_KEY);
    if(d_postal_code.empty()){
        BESDEBUG(MODULE,__func__ << "() -  The configuration entry for the " <<
            SERVER_ADMINISTRATOR_KEY << "[" << POSTAL_CODE_KEY << "] was missing." << endl);
        bad_flag = true;
    }

    d_country = get(COUNTRY_KEY);
    if(d_country.empty()){
        BESDEBUG(MODULE,__func__ << "() -  The configuration entry for the " <<
            SERVER_ADMINISTRATOR_KEY << "[" << COUNTRY_KEY << "] was missing." << endl);
        bad_flag = true;
    }

    d_telephone = get(TELEPHONE_KEY);
    if(d_telephone.empty()){
        BESDEBUG(MODULE,__func__ << "() -  The configuration entry for the " <<
            SERVER_ADMINISTRATOR_KEY << "[" << TELEPHONE_KEY << "] was missing." << endl);
        bad_flag = true;
    }

    d_email = get(EMAIL_KEY);
    if(d_email.empty()){
        BESDEBUG(MODULE,__func__ << "() -  The configuration entry for the " <<
            SERVER_ADMINISTRATOR_KEY << "[" << EMAIL_KEY << "] was missing." << endl);
        bad_flag = true;
    }

    d_website = get(WEBSITE_KEY);
    if(d_website.empty()){
        BESDEBUG(MODULE,__func__ << "() -  The configuration entry for the " <<
            SERVER_ADMINISTRATOR_KEY << "[" << WEBSITE_KEY << "] was missing." << endl);
        bad_flag = true;
    }

    // %TODO This is a pretty simple (and brutal) qc in that any missing value prompts all of it to be rejected. Review. Fix?
    if(bad_flag ){
        mk_default();
        BESDEBUG(MODULE,__func__ << "() -  The configuration entry for the " << SERVER_ADMINISTRATOR_KEY << " was missing crucial information.  jdump(): " << jdump(true) << endl);
    }
}



std::string ServerAdministrator::get(const string &key){
    string lkey = BESUtil::lowercase(key);
    auto result = d_admin_info.find(lkey);
    if(result == d_admin_info.end()){
        return "";
    }
    return result->second;
}





std::string ServerAdministrator::xdump() const {
    std::stringstream ss;
    ss << R"(<ServerAdministrator )";
    ss << R"(organization=")" << d_organization << R"(" )";
    ss <<       R"(street=")" << d_street << R"(" )";
    ss <<         R"(city=")" << d_city << R"(" )";
    ss <<       R"(region=")" << d_region << R"(" )";
    ss <<      R"(country=")" << d_country << R"(" )";
    ss <<   R"(postalcode=")" << d_postal_code << R"(" )";
    ss <<    R"(telephone=")" << d_telephone << R"(" )";
    ss <<        R"(email=")" << d_email << R"(" )";
    ss <<      R"(website=")" << d_website << R"(" )";
    ss << "/>";
    return ss.str();
}

std::string ServerAdministrator::jdump(bool compact) const {
    std::stringstream ss;
    ss  << "{";
    if(!compact)
        ss<< endl << "  ";
    ss << R"("ServerAdministrator":)";
    if(!compact)
        ss << " ";
    ss << "{";
    if(!compact) ss << " ";
    if(!compact) ss << " ";

    if(!compact){ ss << endl << "   "; }
    ss << R"("organization": ")" << d_organization << R"(", )";
    if(!compact){ ss << endl << "   "; }
    ss <<       R"("street": ")" << d_street << R"(", )";
    if(!compact){ ss << endl << "   "; }
    ss <<         R"("city": ")" << d_city << R"(", )";
    if(!compact){ ss << endl << "   "; }
    ss <<       R"("region": ")" << d_region << R"(", )";
    if(!compact){ ss << endl << "   "; }
    ss <<      R"("country": ")" << d_country << R"(", )";
    if(!compact){ ss << endl << "   "; }
    ss <<   R"("postalcode": ")" << d_postal_code << R"(", )";
    if(!compact){ ss << endl << "   "; }
    ss <<    R"("telephone": ")" << d_telephone << R"(", )";
    if(!compact){ ss << endl << "   "; }
    ss <<        R"("email": ")" << d_email << R"(", )";
    if(!compact){ ss << endl << "   "; }
    ss <<      R"("website": ")" << d_website << R"(" )";
    if(!compact){ ss << endl << "   "; }
    ss << "}";
    if(!compact)
        ss << endl;
    ss << "}";
    if(!compact)
        ss << endl;
    return ss.str();
}


void ServerAdministrator::mk_default() {
    this->d_admin_info.clear();
    d_admin_info.insert( std::pair<string,string>(EMAIL_KEY,EMAIL_DEFAULT));
    d_admin_info.insert( std::pair<string,string>(ORGANIZATION_KEY,ORGANIZATION_DEFAULT));
    d_admin_info.insert( std::pair<string,string>(STREET_KEY,STREET_DEFAULT));
    d_admin_info.insert( std::pair<string,string>(CITY_KEY,CITY_DEFAULT));
    d_admin_info.insert( std::pair<string,string>(REGION_KEY,REGION_DEFAULT));
    d_admin_info.insert( std::pair<string,string>(POSTAL_CODE_KEY,POSTAL_CODE_DEFAULT));
    d_admin_info.insert( std::pair<string,string>(COUNTRY_KEY,COUNTRY_DEFAULT));
    d_admin_info.insert( std::pair<string,string>(TELEPHONE_KEY,TELEPHONE_DEFAULT));
    d_admin_info.insert( std::pair<string,string>(WEBSITE_KEY,WEBSITE_DEFAULT));
    BESDEBUG(MODULE,__func__ << "() - ServerAdministrator values have been set to the defaults:  " << jdump(true) << endl);
}





} // namespace bes
