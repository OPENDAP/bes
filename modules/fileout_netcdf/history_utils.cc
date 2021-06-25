// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the Hyrax data server.

// Copyright (c) 2021 OPeNDAP, Inc.
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <exception>
#include <sstream>      // std::stringstream
#include <thread>
#include <future>

#include <D4Group.h>
#include <D4Attributes.h>
#include <DataDDS.h>
#include <escaping.h>

#include <TheBESKeys.h>
#include <BESContextManager.h>
#include <BESDataDDSResponse.h>
#include <BESDapNames.h>
#include <BESDataNames.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <TempFile.h>

#include <BESDapResponseBuilder.h>

#include <BESLog.h>
#include <BESError.h>
#include <BESDapError.h>
#include <BESForbiddenError.h>
#include <BESInternalFatalError.h>
#include <DapFunctionUtils.h>
#include <stringbuffer.h>
#include <writer.h>
#include "document.h"

#include "FONcBaseType.h"
#include "FONcRequestHandler.h"
#include "FONcTransmitter.h"
#include "FONcTransform.h"

using namespace std;
using namespace rapidjson;

#define MODULE "fonc"
#define prolog string("history_utils::").append(__func__).append("() - ")

void appendHistoryJson(vector<string> *global_attr, vector<string> jsonNew)
{

    const char *oldJson = global_attr->at(0).c_str();
    const char *newJson = jsonNew.at(0).c_str();
    Document docNew, docOld;
    Document::AllocatorType &allocator = docOld.GetAllocator();
    docNew.SetArray();
    docNew.Parse(newJson);
    docOld.SetArray();
    docOld.Parse(oldJson);
    docNew.PushBack(docOld, allocator);

    // Stringify JSON
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    docNew.Accept(writer);
    global_attr->clear();
    global_attr->push_back(buffer.GetString());
}



/**
 * @brief Build a history entry. Used only if the cf_history_context is not set.
 *
 * @param request_url The request URL to add to the history value
 * @return A history value string. The caller must actually add this to a 'history'
 * attribute, etc.
 */
string create_cf_history_txt(const string &request_url)
{
    // This code will be used only when the 'cf_history_context' is not set,
    // which should be never in an operating server. However, when we are
    // testing, often only the besstandalone code is running and the existing
    // baselines don't set the context, so we have this. It must do something
    // so the tests are not hopelessly obscure and filter out junk that varies
    // by host (e.g., the names of cached files that have been decompressed).
    // jhrg 6/3/16

    string cf_history_entry;
    std::stringstream ss;
    time_t raw_now;
    struct tm *timeinfo;
    time(&raw_now); /* get current time; same as: timer = time(NULL)  */
    timeinfo = localtime(&raw_now);

    char time_str[100];
    strftime(time_str, 100, "%Y-%m-%d %H:%M:%S", timeinfo);

    ss << time_str << " " << "Hyrax" << " " << request_url;
    cf_history_entry = ss.str();
    BESDEBUG(MODULE, prolog << "Adding cf_history_entry context. '" << cf_history_entry << "'" << endl);
    return cf_history_entry;
}

/**
 * @brief Build a history_json entry. Used only if the history_json_context is not set.
 *
 * @param request_url The request URL to add to the history value
 * @return A history_json value string. The caller must actually add this to a 'history_json'
 * attribute, etc.
 */
template <typename Writer>
void create_json_history_obj(const string &request_url, Writer& writer)
{
    // This code will be used only when the 'history_json_context' is not set,
    // which should be never in an operating server. However, when we are
    // testing, often only the besstandalone code is running and the existing
    // baselines don't set the context, so we have this. It must do something
    // so the tests are not hopelessly obscure and filter out junk that varies
    // by host (e.g., the names of cached files that have been decompressed).
    // jhrg 6/3/16
    // sk 6/17/21

    // "$schema"
    string schema = "https://harmony.earthdata.nasa.gov/schemas/history/0.1.0/history-0.1.0.json";
    // "date_time"
    time_t raw_now;
    struct tm *timeinfo;
    time(&raw_now); /* get current time; same as: timer = time(NULL)  */
    timeinfo = localtime(&raw_now);
    char time_str[100];
    strftime(time_str, 100, "%Y-%m-%dT%H:%M:%S", timeinfo);

    writer.StartObject();
    writer.Key("$schema");
    writer.String(schema.c_str());
    writer.Key("date_time");
    writer.String(time_str);
    writer.Key("program");
    writer.String("hyrax");
    writer.Key("version");
    writer.String("1.16.3");
    writer.Key("parameters");
    writer.StartArray();
    writer.StartObject();
    writer.Key("request_url");
    writer.String(request_url.c_str());
    writer.EndObject();
    writer.EndArray();
    writer.EndObject();
}

/**
* Gets the "cf_history" attribute context.
*
* @request_url
*/
string get_cf_history_entry (const string &request_url)
{
    bool foundIt = false;
    string cf_history_entry = BESContextManager::TheManager()->get_context("cf_history_entry", foundIt);
    if (!foundIt) {
        // If the cf_history_entry context was not set by the incoming command then
        // we compute and the value of the history string here.
        cf_history_entry = create_cf_history_txt(request_url);
    }
    return cf_history_entry;
}

/**
* @brief Gets the "history_json" entry for this request.
*
* @request_url
*/
vector<string> get_history_json_entry (const string &request_url)
{
    vector<string> history_json_entry_vec;
    bool foundIt = false;
    string history_json_entry = BESContextManager::TheManager()->get_context("history_json_entry", foundIt);

    if (!foundIt) {
        // If the history_json_entry context was not set by the incoming command then
        // we compute and the value of the history string here.
        Document  history_json_doc;
        history_json_doc.SetObject();
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        create_json_history_obj(request_url, writer);
        history_json_entry = buffer.GetString();
    }

    BESDEBUG(MODULE,prolog << "Using history_json_entry: " << history_json_entry << endl);
    // And here we add to the returned vector.
    history_json_entry_vec.push_back(history_json_entry);
    return history_json_entry_vec;
}








/**
 * @brief Makes a history_json entry using the request URL with constraint expression
 * @param request_url The request URL including the constraint expression (query string)
 * @return A history_json entry for this request url.
 */
string get_hj_entry (const string &request_url)
{
    bool foundIt = false;
    string history_json_entry = BESContextManager::TheManager()->get_context("history_json_entry", foundIt);
    if (!foundIt) {
        // If the history_json_entry context was not set as a context key on BESContextManager
        // we compute and the value of the history string here.
        Document  history_json_doc;
        history_json_doc.SetObject();
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        create_json_history_obj(request_url, writer);
        history_json_entry = buffer.GetString();
    }

    BESDEBUG(MODULE,prolog << "Using history_json_entry: " << history_json_entry << endl);
    return history_json_entry;
}

/**
 *
 * @param current_doc_str
 * @param new_entry_str
 * @return
 */
string json_append_entry_to_array(const string& current_doc_str, const string& new_entry_str)
{
    Document hj_doc, new_hj_doc;
    hj_doc.SetArray();
    Document::AllocatorType &allocator = hj_doc.GetAllocator();

    hj_doc.Parse(current_doc_str.c_str()); // Parse attribute from file

    new_hj_doc.Parse(new_entry_str.c_str()); // Parse frontend entry

    hj_doc.PushBack(new_hj_doc, allocator);

    // Stringify JSON
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    hj_doc.Accept(writer);
    return buffer.GetString();
}

/**
 * @brief Updates/Creates a (NASA) history_json attribute in global_attribute.
 * @param global_attribute
 * @param request_url
 */
void update_history_json_attr(D4Attribute *global_attribute, const string &request_url)
{
    string hj_entry_str = get_hj_entry(request_url);
    BESDEBUG(MODULE,prolog << "hj_entry: " << hj_entry_str << endl);

    D4Attribute *history_json_attr = global_attribute->attributes()->find("history_json");
    if (!history_json_attr) {
        // If there is no source history_json attribute then we make one from scratch
        // and add it to the global_attribute
        BESDEBUG(MODULE, prolog << "Adding history_json entry to global_attribute " << global_attribute->name() << endl);
        string hj_array_str = "[" + hj_entry_str +"]";
        vector<string> attr_vals;
        attr_vals.push_back(hj_array_str);
        auto new_history_json = new D4Attribute("history_json", attr_str_c);
        new_history_json->add_value_vector(attr_vals);
        global_attribute->attributes()->add_attribute_nocopy(new_history_json);
    } else {
        // We found an existing history_jason attribute!
        // We know the convention is that this should be a single valued DAP attribute
        // We need to get the existing json document, parse it, insert the entry into
        // the document using rapidjson, and then serialize it to a new string value that
        // We will use to overwrite the current value in the existing history_json_attr.
        string history_json = *history_json_attr->value_begin();
        history_json=R"([{"$schema":"https:\/\/harmony.earthdata.nasa.gov\/schemas\/history\/0.1.0\/history-0.1.0.json","date_time":"2021-06-25T13:28:48.951+0000","program":"hyrax","version":"@HyraxVersion@","parameters":[{"request_url":"http:\/\/localhost:8080\/opendap\/hj\/coads_climatology.nc.dap.nc4?GEN1"}]}])";
        BESDEBUG(MODULE,prolog << "FOUND history_json: " << history_json << endl);

        history_json = json_append_entry_to_array(history_json, hj_entry_str);
        BESDEBUG(MODULE,prolog << "NEW history_json: " << history_json << endl);

        // Now the we have the update history_json element, serialized to a string, we use it to
        // the value of the existing D4Attribute history_json_attr
        vector<string> attr_vals;
        attr_vals.push_back(history_json);

        // This replaces the value
        history_json_attr->add_value_vector(attr_vals);
    }
}

/**
 * @brief Updates/Creates a climate forecast (cf) history attribute in global_attribute.
 * @param global_attribute
 * @param request_url
 */
void update_cf_history_attr(D4Attribute *global_attribute, const string &request_url){

    string cf_hist_entry = get_cf_history_entry(request_url);
    BESDEBUG(MODULE, prolog << "cf_hist_entry: " << cf_hist_entry << endl);

    D4Attribute *history_attr = global_attribute->attributes()->find("history");
    if (!history_attr) {
        //if there is no source history attribute
        BESDEBUG(MODULE, prolog << "Adding history entry to " << global_attribute->name() << endl);
        auto *new_history = new D4Attribute("history", attr_str_c);
        std::vector<std::string> cf_hist_entry_vec;
        cf_hist_entry_vec.push_back(cf_hist_entry);
        new_history->add_value_vector(cf_hist_entry_vec);
        global_attribute->attributes()->add_attribute_nocopy(new_history);
    } else {
        string newline("\n");
        stringstream cf_hist;
        cf_hist << history_attr->value(0);
        if(!BESUtil::endsWith(cf_hist.str(),newline))
            cf_hist << endl;
        cf_hist << cf_hist_entry << endl;
        BESDEBUG(MODULE, prolog << "cf_hist: " << cf_hist.str() << endl);

        std::vector<std::string> cf_hist_vec;
        cf_hist_vec.push_back(cf_hist.str());
        history_attr->add_value_vector(cf_hist_vec);
    }
}

/**
* Process the DAP4 "history" attribute.
*
* @param dmr The DMR to modify
* @param ce The constraint expression that produced this new netCDF file.
*/
void updateHistoryAttribute(DMR *dmr, const string &ce)
{
    string request_url = dmr->filename();
    // remove path info
    request_url = request_url.substr(request_url.find_last_of('/')+1);
    // remove 'uncompress' cache mangling
    request_url = request_url.substr(request_url.find_last_of('#')+1);
    if(!ce.empty()) request_url += "?" + ce;

    bool added_history = false;
    D4Group* root_grp = dmr->root();
    D4Attributes *root_attrs = root_grp->attributes();
    for (auto attrs = root_attrs->attribute_begin(); attrs != root_attrs->attribute_end(); ++attrs) {
        string name = (*attrs)->name();
        BESDEBUG(MODULE, prolog << "Attribute name is "<< name << endl);
        if ((*attrs)->type() && BESUtil::endsWith(name, "_GLOBAL")) {
            // Update Climate Forecast history attribute.
            update_cf_history_attr(*attrs, request_url);
            // Update NASA's history_json attribute
            update_history_json_attr(*attrs, request_url);
            added_history = true;
        }
    }
    if(!added_history){
        auto *dap_global = new D4Attribute("DAP_GLOBAL",attr_container_c);
        root_attrs->add_attribute_nocopy(dap_global);
        // CF history attribute
        update_cf_history_attr(dap_global, request_url);
        // NASA's history_json attribute
        update_history_json_attr(dap_global,request_url);
    }
}


/**
 * Process the DAP2 "history" attribute.
 *
 * @param dds The DDS to modify
 * @param ce The constraint expression that produced this new netCDF file.
 */
void updateHistoryAttribute(DDS *dds, const string &ce)
{
    string request_url = dds->filename();
    // remove path info
    request_url = request_url.substr(request_url.find_last_of('/')+1);
    // remove 'uncompress' cache mangling
    request_url = request_url.substr(request_url.find_last_of('#')+1);
    if(!ce.empty()) request_url += "?" + ce;

    std::vector<std::string> cf_hist_entry_vec;
    cf_hist_entry_vec.push_back(get_cf_history_entry(request_url));

    BESDEBUG(MODULE, prolog << "cf_hist_entry_vec.size(): " << cf_hist_entry_vec.size() << endl);

    std::vector<std::string> history_json_entry_vec = get_history_json_entry(request_url);
    BESDEBUG(MODULE, prolog << "hist_json_entry_vec.size(): " << history_json_entry_vec.size() << endl);

    // Add the new entry to the "history" attribute
    // Get the top level Attribute table.
    AttrTable &globals = dds->get_attr_table();

    // Since many files support "CF" conventions the history tag may already exist in the source data
    // and we should add an entry to it if possible.
    bool added_history = false; // Used to indicate that we located a toplevel AttrTable whose name ends in "_GLOBAL" and that has an existing "history" attribute.
//    unsigned int num_attrs = globals.get_size();
    if (globals.is_global_attribute()) {
        // Here we look for a top level AttrTable whose name ends with "_GLOBAL" which is where, by convention,
        // data ingest handlers place global level attributes found in the source dataset.
        auto i = globals.attr_begin();
        auto e = globals.attr_end();
        for (; i != e; i++) {
            AttrType attrType = globals.get_attr_type(i);
            string attr_name = globals.get_name(i);
            // Test the entry...
            if (attrType == Attr_container && BESUtil::endsWith(attr_name, "_GLOBAL")) {
                // We are going to append to an existing history attribute if there is one
                // Or just add a history attribute if there is not one. In a most
                // handy API moment, append_attr() does just this.

                AttrTable *global_attr_tbl = globals.get_attr_table(i);
                global_attr_tbl->append_attr("history", "string", &cf_hist_entry_vec);

                vector<string> *history_json_attr_vector = global_attr_tbl->get_attr_vector("history_json");
                if (!history_json_attr_vector) {
                    //if there is no source history_json attribute
                    BESDEBUG(MODULE, prolog << "Adding history_json entry to " << attr_name << endl);
                    global_attr_tbl->append_attr("history_json", "string", &history_json_entry_vec);
                } else {
                    appendHistoryJson(history_json_attr_vector, history_json_entry_vec);
                }

                added_history = true;
                BESDEBUG(MODULE, prolog << "Added history entries to " << attr_name << endl);
            }
        }
        if(!added_history){
            auto dap_global_at = globals.append_container("DAP_GLOBAL");
            dap_global_at->set_name("DAP_GLOBAL");

            dap_global_at->append_attr("history", "string", &cf_hist_entry_vec);
            BESDEBUG(MODULE, prolog << "No top level AttributeTable name matched '*_GLOBAL'. "
                                       "Created DAP_GLOBAL AttributeTable and added history attributes to it." << endl);
            dap_global_at->append_attr("history_json", "string", &history_json_entry_vec);
        }
    }
}

