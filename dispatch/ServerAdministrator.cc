
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

#include <TheBESKeys.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include "BESInternalFatalError.h"

#include "ServerAdministrator.h"

using std::vector;

#define MODULE "bes"

#define prolog std::string("ServerAdministrator::").append(__func__).append("() - ")


namespace bes {

ServerAdministrator::ServerAdministrator(){
    bool found = false;
    vector<string> admin_keys;
    TheBESKeys::TheKeys()->get_values(SERVER_ADMINISTRATOR_KEY, admin_keys, found);
    if(!found){
        throw BESInternalFatalError(string("The BES configuration must provide server administrator information using the key: '")+SERVER_ADMINISTRATOR_KEY
            +"'", __FILE__, __LINE__);
    }

    vector<string>::iterator it;
    for(it=admin_keys.begin();  it!=admin_keys.end(); it++){
        string admin_info_entry = *it;
        int index = admin_info_entry.find(":");
        if(index>0){
            string key = admin_info_entry.substr(0,index);
            string value =  admin_info_entry.substr(index+1);
            BESDEBUG(MODULE, prolog << "key: '" << key << "'  value: " << value << endl);
            d_admin_info.insert( std::pair<string,string>(key,value));
        }
        else {
            throw BESInternalFatalError(string("The configuration entry for the ") + SERVER_ADMINISTRATOR_KEY +
                " was incorrectly formatted. entry: "+admin_info_entry, __FILE__,__LINE__);
        }
    }
}

std::string ServerAdministrator::get(const string &key){
    string lkey = BESUtil::lowercase(key);
    std::map<std::string,std::string>::const_iterator result = d_admin_info.find(lkey);
    if(result == d_admin_info.end()){
        return "";
    }
    return result->second;
}

std::string ServerAdministrator::xdump() const {
    std::stringstream ss;
    std::map<std::string,std::string>::const_iterator it = d_admin_info.begin();
    ss << "<ServerAdministrator ";
    for(it=d_admin_info.begin(); it!= d_admin_info.end(); it++){
        if(it!= d_admin_info.begin())
            ss << " ";
        ss << it->first << "=\"" << it->second << "\"";
    }
    ss << "/>";
    return ss.str();
}

std::string ServerAdministrator::jdump(bool compact) const {
    std::stringstream ss;
    std::map<std::string,std::string>::const_iterator it = d_admin_info.begin();
    ss  << "{";
    if(!compact)
        ss<< endl << "  ";
    ss << "\"ServerAdministrator\":";
    if(!compact)
        ss << " ";
    ss << "{";
    if(!compact) ss << " ";
    if(!compact) ss << " ";
    for(it=d_admin_info.begin(); it!= d_admin_info.end(); it++){
        if(it!= d_admin_info.begin())
            ss << ",";
        if(!compact)
            ss << endl << "   ";
        ss << "\"" << it->first << "\"" << ":";
        if(!compact)
            ss << " ";
        ss << "\"" << it->second << "\"";
    }
    if(!compact)
        ss<< endl << "  ";
    ss << "}";
    if(!compact)
        ss << endl;
    ss << "}";
    if(!compact)
        ss << endl;
    return ss.str();
}


} // namespace bes
