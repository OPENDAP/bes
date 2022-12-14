// -*- mode: c++; c-basic-offset:4 -*-
//
// This file is part of cmr_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.
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
//
#include <sstream>
#include <fstream>

#include "nlohmann/json.hpp"

#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "RemoteResource.h"

#include "CmrNames.h"
#include "CmrNotFoundError.h"
#include "CmrInternalError.h"

#include "JsonUtils.h"

using namespace std;
using json = nlohmann::json;

#define prolog std::string("JsonUtils::").append(__func__).append("() - ")

std::string truth(bool t){ if(t){return "true";} return "false"; }

namespace cmr {
/**
 * Utilizes the RemoteHttpResource machinery to retrieve the document
 * referenced by the parameter 'url'. Once retrieved the document is fed to the RapidJSON
 * parser to populate the parameter 'd'
 *
 * @param url The URL of the JSON document to parse.
 * @return The json document parsed from the source URL response..
 *
 */
json JsonUtils::get_as_json(const string &url)
{
    BESDEBUG(MODULE,prolog << "Trying url: " << url << endl);
    shared_ptr<http::url> target_url(new http::url(url));
    http::RemoteResource remoteResource(target_url);
    remoteResource.retrieveResource();
    std::ifstream f(remoteResource.getCacheFileName());
    json data = json::parse(f);
    return data;
}

std::string JsonUtils::typeName(unsigned int t)
{
    const char* tnames[] =
            { "Null", "False", "True", "Object", "Array", "String", "Number" };
    return string(tnames[t]);
}

double JsonUtils::qc_double(const std::string &key, const nlohmann::json &json_obj) const
{
    double value=0.0;

    BESDEBUG(MODULE, prolog << "Key: '" << key << "' JSON: " << endl << json_obj.dump(2) << endl);
    // Check input for object.
    bool result = json_obj.is_object();
    string msg0 = prolog + "Json document is" + (result?"":" NOT") + " an object.";
    BESDEBUG(MODULE, msg0 << endl);
    if(!result){
        return value;
    }
    const auto &key_itr = json_obj.find(key);
    if(key_itr == json_obj.end()){
        stringstream msg;
        msg << prolog;
        msg << "Ouch! Unable to locate the '" << key;
        msg << "' child of json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        return value;
    }

    auto &dobj = json_obj[key];
    if(dobj.is_null()){
        stringstream msg;
        msg << prolog;
        msg << "Failed to locate the '" << key;
        msg << "' child of json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        return value;
    }
    if(!dobj.is_number_float() && !dobj.is_number()){
        stringstream msg;
        msg << prolog;
        msg << "The child element called '" << key;
        msg << "' is not a string. json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        return value;
    }
    return dobj.get<double>();


}

unsigned long  JsonUtils::qc_int(const std::string &key, const nlohmann::json &json_obj) const
{
    double value=0.0;

    BESDEBUG(MODULE, prolog << "Key: '" << key << "' JSON: " << endl << json_obj.dump(2) << endl);
    // Check input for object.
    bool result = json_obj.is_object();
    string msg0 = prolog + "Json document is" + (result?"":" NOT") + " an object.";
    BESDEBUG(MODULE, msg0 << endl);
    if(!result){
        return value;
    }
    const auto &key_itr = json_obj.find(key);
    if(key_itr == json_obj.end()){
        stringstream msg;
        msg << prolog;
        msg << "Ouch! Unable to locate the '" << key;
        msg << "' child of json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        return value;
    }

    auto &dobj = json_obj[key];
    if(dobj.is_null()){
        stringstream msg;
        msg << prolog;
        msg << "Failed to locate the '" << key;
        msg << "' child of json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        return value;
    }
    if(!dobj.is_number_float() && !dobj.is_number()){
        stringstream msg;
        msg << prolog;
        msg << "The child element called '" << key;
        msg << "' is not a string. json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        return value;
    }
    return dobj.get<double>();


}

/**
 *
 * @param key
 * @param json_obj
 * @return
 */
std::string JsonUtils::get_str_if_present(const std::string &key, const nlohmann::json& json_obj) const
{
    string value;

    BESDEBUG(MODULE, prolog << "Key: '" << key << "' JSON: " << endl << json_obj.dump(2) << endl);
    // Check input for object.
    bool result = json_obj.is_object();
    string msg0 = prolog + "Json document is" + (result?"":" NOT") + " an object.";
    BESDEBUG(MODULE, msg0 << endl);
    if(!result){
        return value;
    }

    const auto &key_itr = json_obj.find(key);
    if(key_itr == json_obj.end()){
        stringstream msg;
        msg << prolog;
        msg << "Ouch! Unable to locate the '" << key;
        msg << "' child of json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        return value;
    }

    auto &string_obj = json_obj[key];
    if(string_obj.is_null()){
        stringstream msg;
        msg << prolog;
        msg << "Failed to locate the '" << key;
        msg << "' child of json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        return value;
    }
    if(!string_obj.is_string()){
        stringstream msg;
        msg << prolog;
        msg << "The child element called '" << key;
        msg << "' is not a string. json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        return value;
    }
    return string_obj.get<string>();


}


const nlohmann::json& JsonUtils::qc_get_array(const std::string &key, const nlohmann::json& json_obj) const
{
    BESDEBUG(MODULE, prolog << "Key: '" << key << "' JSON: " << endl << json_obj.dump(2) << endl);
    // Check input for object.
    bool result = json_obj.is_object();
    string msg0 = prolog + "Json document is" + (result?"":" NOT") + " an object.";
    BESDEBUG(MODULE, msg0 << endl);
    if(!result){
        throw CmrInternalError(msg0, __FILE__, __LINE__);
    }

    const auto &key_itr = json_obj.find(key);
    if(key_itr == json_obj.end()){
        stringstream msg;
        msg << prolog;
        msg << "Ouch! Unable to locate the '" << key;
        msg << "' child of json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrNotFoundError(msg.str(), __FILE__, __LINE__);
    }

    auto &array_obj = json_obj[key];
    if(array_obj.is_null()){
        stringstream msg;
        msg << prolog;
        msg << "Ouch! Unable to locate the '" << key;
        msg << "' child of json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrNotFoundError(msg.str(), __FILE__, __LINE__);
    }
    if(!array_obj.is_array()){
        stringstream msg;
        msg << prolog;
        msg << "ERROR: The child element called '" << key;
        msg << "' is not an array. json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrInternalError(msg.str(), __FILE__, __LINE__);
    }
    return array_obj;
}
const nlohmann::json& JsonUtils::qc_get_object(const std::string &key, const nlohmann::json& json_obj) const
{
    BESDEBUG(MODULE, prolog << "Key: '" << key << "' JSON: " << endl << json_obj.dump(2) << endl);

    // Check input for object.
    bool result = json_obj.is_object();
    string msg0 = prolog + "Json document is" + (result?"":" NOT") + " an object.";
    BESDEBUG(MODULE, msg0 << endl);
    if(!result){
        throw CmrInternalError(msg0, __FILE__, __LINE__);
    }

    const auto &key_itr = json_obj.find(key);
    if(key_itr == json_obj.end()){
        stringstream msg;
        msg << prolog;
        msg << "Ouch! Unable to locate the '" << key;
        msg << "' child of json: " << endl << json_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrNotFoundError(msg.str(), __FILE__, __LINE__);
    }

    auto &child_obj = json_obj[key];

    if(child_obj.is_null()){
        stringstream msg;
        msg << prolog;
        msg << "Ouch! Unable to locate the '" << key;
        msg << "' child of json: " << endl << child_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrNotFoundError(msg.str(), __FILE__, __LINE__);
    }
    if(!child_obj.is_object()){
        stringstream msg;
        msg << prolog;
        msg << "ERROR: The child element called '" << key;
        msg << "' is not an object. json: " << endl << child_obj.dump(2) << endl;
        BESDEBUG(MODULE, msg.str() << endl);
        throw CmrInternalError(msg.str(), __FILE__, __LINE__);
    }
    return child_obj;
}

/**
 *
 * @param j
 * @return
 */
std::string JsonUtils::probe_json(const nlohmann::json &j) const
{
    string hdr0("#########################################################################################");
    string hdr1("#- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -");

    stringstream msg;
    msg << endl;
    msg << hdr0 << endl;
    msg << j.dump(2) << endl;
    msg << hdr1 << endl;
    msg << "           j.is_null(): " << truth(j.is_null()) << endl;
    msg << "         j.is_object(): " << truth(j.is_object()) << endl;
    msg << "          j.is_array(): " << truth(j.is_array()) << endl;
    msg << endl;
    msg << "      j.is_discarded(): " << truth(j.is_discarded()) << endl;
    msg << "         j.is_string(): " << truth(j.is_string()) << endl;
    msg << "     j.is_structured(): " << truth(j.is_structured()) << endl;
    msg << "         j.is_binary(): " << truth(j.is_binary()) << endl;
    msg << "        j.is_boolean(): " << truth(j.is_boolean()) << endl;
    msg << "         j.is_number(): " << truth(j.is_number()) << endl;
    msg << "   j.is_number_float(): " << truth(j.is_number_float()) << endl;
    msg << " j.is_number_integer(): " << truth(j.is_number_integer()) << endl;
    msg << "j.is_number_unsigned(): " << truth(j.is_number_unsigned()) << endl;
    msg << "      j.is_primitive(): " << truth(j.is_primitive()) << endl;
    msg << "              j.size(): " << j.size()<< endl;
    msg << "             j.empty(): " << truth(j.empty()) << endl;

    msg << hdr0 << endl;
    return msg.str();
}



}  // namespace cmr
