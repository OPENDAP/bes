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

#include <fstream>
#include <sstream>      // std::stringstream
#include <thread>
#include <future>

// rapidjson
#include <stringbuffer.h>
#include <writer.h>
#include <document.h>

#include <libdap/DDS.h>
#include <libdap/DMR.h>
#include <libdap/D4Group.h>
#include <libdap/D4Attributes.h>
#include <libdap/DataDDS.h>

#include "BESContextManager.h"
#include "BESDapResponseBuilder.h"
#include "DapFunctionUtils.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "TempFile.h"

#include "FONcBaseType.h"
#include "FONcTransmitter.h"
#include "FONcTransform.h"

using namespace std;

#define NEW_LINE ((char)0x0a)
#define CF_HISTORY_KEY "history"
#define CF_HISTORY_CONTEXT "cf_history_entry"
#define HISTORY_JSON_KEY "history_json"
#define HISTORY_JSON_CONTEXT "history_json_entry"

// Define this to keep the JSON history attribute out of the DAS and
// drop it into the netCDF file directly.
//
// Look in FONcTransform.cc for *** in a comment for the locations
// where the change could be made. For now, the current approach, which
// is sort of convoluted, is working. jhrg 2/28/22
#define HISTORY_JSON_DIRECT_TO_NETCDF 0

#define MODULE "fonc"
#define prolog string("history_utils::").append(__func__).append("() - ")

// I added this namespace because only two of these functions are called by
// code outside this file and its associated unit test. jhrg 2/25/22

namespace fonc_history_util {

/**
 * @brief Build a string representing the local time now
 * @return The encoded date-time as a C++ string
 */
string
get_time_now() {
    time_t raw_now;
    // jhrg 2/2/24 struct tm *timeinfo;
    time(&raw_now); /* get current time; same as: timer = time(NULL)  */
    const struct tm *timeinfo = localtime(&raw_now);

    char time_str[128];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
    return string(time_str);
}

/**
 * @brief Build a history entry. Used only if the cf_history_context is not set.
 *
 * This function builds a single 'line' in a history attribute that is intended to
 * indicate that a hyrax server was used to access the 'request_url' at a certain
 * time.
 *
 * @param request_url The request URL to add to the history value
 * @return A history value string. The caller must actually add this to a 'history'
 * attribute, etc.
 */
string create_cf_history_txt(const string &request_url) {
    // This code will be used only when the 'cf_history_context' is not set,
    // which should be never in an operating server. However, when we are
    // testing, often only the besstandalone code is running and the existing
    // baselines don't set the context, so we have this. It must do something
    // so the tests are not hopelessly obscure and filter out junk that varies
    // by host (e.g., the names of cached files that have been decompressed).
    // jhrg 6/3/16

    stringstream ss;
    ss << get_time_now() << " " << "Hyrax" << " " << request_url << NEW_LINE;

    BESDEBUG(MODULE, prolog << "New cf history entry: '" << ss.str() << "'" << endl);
    return ss.str();
}

/**
 * @brief Build a history_json entry. Used only if the history_json_context is not set.
 *
 * @note This code will be used only when the 'history_json_context' is not set,
 * which should be never in an operating server. However, when we are
 * testing, often only the besstandalone code is running and the existing
 * baselines don't set the context, so we have this. It must do something
 * so the tests are not hopelessly obscure and filter out junk that varies
 * by host (e.g., the names of cached files that have been decompressed).
 *
 * @param request_url The request URL to add to the history value
 * @param writer a rapid_json Writer that can encode the new information. The JSON information
 * is returned to the caller using this parameter.
 */
template<typename RJSON_WRITER>
void create_json_history_obj(const string &request_url, RJSON_WRITER &writer) {
    const string schema = "https://harmony.earthdata.nasa.gov/schemas/history/0.1.0/history-0.1.0.json";

    writer.StartObject();
    writer.Key("$schema");
    writer.String(schema.c_str());
    writer.Key("date_time");
    writer.String(get_time_now().c_str() /*jhrg time_str*/);
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

// NB: This is where I stopped writing unit tests. jhrg 2/25/22

/**
 * @brief Get the CF History entry
 *
 * This function looks for the CF History context passed from the Hyrax front end
 * _or_ it builds a value using the 'request_url' parameter and the current time.
 *
 * @param request_url
 * @return The new CF History entry.
 */
static string get_cf_history_entry(const string &request_url) {
    bool foundIt = false;
    string cf_history_entry = BESContextManager::TheManager()->get_context(CF_HISTORY_CONTEXT, foundIt);
    if (!foundIt) {
        // If the cf_history_entry context was not set by the incoming command then
        // we compute and the value of the history string here.
        cf_history_entry = create_cf_history_txt(request_url);
    }
    return cf_history_entry;
}

/**
 * @brief Retrieves the history_json entry from the BESContextManager or lacking that creates a new one using the request URL.
 * @param request_url The request URL including the constraint expression (query string)
 * @return A history_json entry for this request url.
 */
static string get_history_json_entry(const string &request_url) {
    bool foundIt = false;
    string history_json_entry = BESContextManager::TheManager()->get_context(HISTORY_JSON_CONTEXT, foundIt);
    if (!foundIt) {
        // If the history_json_entry context was not set as a context key on BESContextManager
        // we compute and the value of the history string here.
        rapidjson::Document history_json_doc;
        history_json_doc.SetObject();
        rapidjson::StringBuffer buffer;
        rapidjson::Writer <rapidjson::StringBuffer> writer(buffer);
        create_json_history_obj(request_url, writer);
        history_json_entry = buffer.GetString();
    }

    BESDEBUG(MODULE, prolog << "Using history_json_entry: " << history_json_entry << endl);
    return history_json_entry;
}

/**
 * @brief Adds the new_entry_str JSON as a new element to the source_array_str JSON
 *
 * Parses array source_array_str and the new_entry_str as JSON objects. Adds the new entry to the source array
 * and returns the serialized version of the new array.
 *
 * @param source_array_str The current JSON string
 * @param new_entry_str The JSON string to append to source_array_str
 * @return THe updated JSON serialized to a string.
 */
string json_append_entry_to_array(const string &source_array_str, const string &new_entry_str) {
    rapidjson::Document target_array;
    target_array.SetArray();
    rapidjson::Document::AllocatorType &allocator = target_array.GetAllocator();
    target_array.Parse(source_array_str.c_str()); // Parse json array

    rapidjson::Document entry;
    entry.Parse(new_entry_str.c_str()); // Parse new entry

    target_array.PushBack(entry, allocator);

    // Stringify JSON
    rapidjson::StringBuffer buffer;
    rapidjson::Writer <rapidjson::StringBuffer> writer(buffer);
    target_array.Accept(writer);
    return buffer.GetString();
}

/**
 * @brief Updates/Creates a (NASA) history_json attribute in global_attribute. (DAP4)
 *
 * This function updates the history JSON attribute for DAP4 using either the
 * information possed into the BES from the OLFS via a BES Context value _or_
 * the time and value of the 'request_url' parameter.
 *
 * If the function cannot find a global attribute named "history_json", that
 * attribute will be made in D4 attribute container held by global_attribute.
 * Regarding finding the history_json attribute, that can eitehr be present in
 * global_attribute's container or actually be the global_attribute.
 *
 * @param global_attribute The global attribute to update.
 * @param request_url
 */
static void update_history_json_attr(D4Attribute *global_attribute, const string &request_url) {
    BESDEBUG(MODULE,
             prolog << "Updating history_json entry for global DAP4 attribute: " << global_attribute->name() << endl);

    string hj_entry_str = get_history_json_entry(request_url);
    BESDEBUG(MODULE, prolog << "hj_entry_str: " << hj_entry_str << endl);

    string history_json;

    D4Attribute *history_json_attr = nullptr;
    if (global_attribute->type() == D4AttributeType::attr_container_c) {
        history_json_attr = global_attribute->attributes()->find(HISTORY_JSON_KEY);
    }
    else if (global_attribute->name() == HISTORY_JSON_KEY) {
        history_json_attr = global_attribute;
    }

    if (!history_json_attr) {
        // If there is no source history_json attribute then we make one from scratch
        // and add it to the global_attribute
        BESDEBUG(MODULE,
                 prolog << "Adding history_json entry to global_attribute " << global_attribute->name() << endl);
        history_json_attr = new D4Attribute(HISTORY_JSON_KEY, attr_str_c);
        global_attribute->attributes()->add_attribute_nocopy(history_json_attr);

        // Promote the entry to a json array, assigning it the value of the attribute
        history_json = "[" + hj_entry_str + "]";
        BESDEBUG(MODULE, prolog << "CREATED history_json: " << history_json << endl);

    }
    else {
        // We found an existing history_jason attribute!
        // We know the convention is that this should be a single valued DAP attribute
        // We need to get the existing json document, parse it, insert the entry into
        // the document using rapidjson, and then serialize it to a new string value that
        // We will use to overwrite the current value in the existing history_json_attr.
        history_json = *history_json_attr->value_begin();

        // This was in the production code, but I think it was left over from early
        // debugging. I'm going to break up the long line so it's more obvious that
        // is the case. jhrg 2/25/22
        // history_json = R"([{"$schema":"https:\/\/harmony.earthdata.nasa.gov\/schemas\/history\/0.1.0\/history-0.1.0.json",
        // "date_time":"2021-06-25T13:28:48.951+0000","program":"hyrax","version":"@HyraxVersion@",
        // "parameters":[{"request_url":"http:\/\/localhost:8080\/opendap\/hj\/coads_climatology.nc.dap.nc4?GEN1"}]}])";

        BESDEBUG(MODULE, prolog << "FOUND history_json: " << history_json << endl);

        // Append the entry to the existing history_json array
        history_json = json_append_entry_to_array(history_json, hj_entry_str);
        BESDEBUG(MODULE, prolog << "NEW history_json: " << history_json << endl);

    }

    // Now that we have the update history_json element, serialized to a string, we use it to
    // replace the value of the existing D4Attribute history_json_attr
    vector <string> attr_vals;
    attr_vals.push_back(history_json);
    history_json_attr->add_value_vector(attr_vals); // This replaces the value
}

/**
 * @brief Appends the cf_history_entry to the existing cf_history, sorting out the newlines as needed.
 *
 * @param cf_history The existing cf_history string.
 * @param cf_history_entry The nes cf history entry to append to cf_history
 * @return The amalgamated result
 */
static string append_cf_history_entry(string cf_history, string cf_history_entry) {

    stringstream cf_hist_new;
    if (!cf_history.empty()) {
        cf_hist_new << cf_history;
        if (cf_history.back() != NEW_LINE)
            cf_hist_new << NEW_LINE;
    }

    cf_hist_new << cf_history_entry;
    if (cf_history_entry.back() != NEW_LINE)
        cf_hist_new << NEW_LINE;

    BESDEBUG(MODULE, prolog << "Updated cf history: '" << cf_hist_new.str() << "'" << endl);
    return cf_hist_new.str();
}

/**
 * @brief Updates/Creates a climate forecast (cf) history attribute in global_attribute. (DAP4)
 *
 * This is similar to the function that manipulates the JSON history attribute.
 * If information about the history of the request is passed into the BES from
 * the front end using a Context, that is used to update the history. If no
 * 'history context' information was sent, then the time and request_url are used
 * to update the history information.
 *
 * As with the other function, an existing history attribute is used if found in
 * global_attribute (which may be a container or an attribute) or a new attribute
 * is made and added to global_attribute.
 *
 * @param global_attribute
 * @param request_url
 * @see update_history_json_attr()
 */
static void update_cf_history_attr(D4Attribute *global_attribute, const string &request_url) {
    BESDEBUG(MODULE,
             prolog << "Updating cf history entry for global DAP4 attribute: " << global_attribute->name() << endl);

    string cf_hist_entry = get_cf_history_entry(request_url);
    BESDEBUG(MODULE, prolog << "New cf history entry: " << cf_hist_entry << endl);

    string cf_history;
    D4Attribute *history_attr = nullptr;
    if (global_attribute->type() == D4AttributeType::attr_container_c) {
        history_attr = global_attribute->attributes()->find(CF_HISTORY_KEY);
    }
    else if (global_attribute->name() == CF_HISTORY_KEY) {
        history_attr = global_attribute;
    }

    if (!history_attr) {
        //if there is no source cf history attribute make one and add it to the global_attribute.
        BESDEBUG(MODULE, prolog << "Adding history entry to " << global_attribute->name() << endl);
        history_attr = new D4Attribute(CF_HISTORY_KEY, attr_str_c);
        global_attribute->attributes()->add_attribute_nocopy(history_attr);
    }
    else {
        cf_history = history_attr->value(0);
    }
    cf_history = append_cf_history_entry(cf_history, cf_hist_entry);

    std::vector <std::string> cf_hist_vec;
    cf_hist_vec.push_back(cf_history);
    history_attr->add_value_vector(cf_hist_vec);
}

/**
 * @brief Updates/Creates a climate forecast (cf) history attribute the in global_attr_tbl. (DAP2)
 *
 * @param global_attribute
 * @param request_url
 */
void update_cf_history_attr(AttrTable *global_attr_tbl, const string &request_url) {

    BESDEBUG(MODULE,
             prolog << "Updating cf history entry for global DAP2 attribute: " << global_attr_tbl->get_name() << endl);

    string cf_hist_entry = get_cf_history_entry(request_url);
    BESDEBUG(MODULE, prolog << "New cf history entry: '" << cf_hist_entry << "'" << endl);

    string cf_history = global_attr_tbl->get_attr(CF_HISTORY_KEY); // returns empty string if not found
    BESDEBUG(MODULE, prolog << "Previous cf history: '" << cf_history << "'" << endl);

    cf_history = append_cf_history_entry(cf_history, cf_hist_entry);
    BESDEBUG(MODULE, prolog << "Updated cf history: '" << cf_history << "'" << endl);

    global_attr_tbl->del_attr(CF_HISTORY_KEY, -1);
    int attr_count = global_attr_tbl->append_attr(CF_HISTORY_KEY, "string", cf_history);
    BESDEBUG(MODULE, prolog << "Found " << attr_count << " value(s) for the cf history attribute." << endl);
}

/**
 * @brief Updates/Creates a NASA history_json attribute in the global_attr_tbl. (DAP2)
 * @param global_attr_tbl
 * @param request_url
 */
void update_history_json_attr(AttrTable *global_attr_tbl, const string &request_url) {

    BESDEBUG(MODULE, prolog << "Updating history_json entry for global DAP2 attribute: " << global_attr_tbl->get_name()
                            << endl);

    string hj_entry_str = get_history_json_entry(request_url);
    BESDEBUG(MODULE, prolog << "New history_json entry: " << hj_entry_str << endl);

    string history_json = global_attr_tbl->get_attr(HISTORY_JSON_KEY);
    BESDEBUG(MODULE, prolog << "Previous history_json: " << history_json << endl);

    if (history_json.empty()) {
        //if there is no source history_json attribute
        BESDEBUG(MODULE,
                 prolog << "Creating new history_json entry to global attribute: " << global_attr_tbl->get_name()
                        << endl);
        history_json = "[" + hj_entry_str + "]"; // Hack to make the entry into a json array.
    }
    else {
        history_json = json_append_entry_to_array(history_json, hj_entry_str);
        global_attr_tbl->del_attr(HISTORY_JSON_KEY, -1);
    }
    BESDEBUG(MODULE, prolog << "New history_json: " << history_json << endl);
    int attr_count = global_attr_tbl->append_attr(HISTORY_JSON_KEY, "string", history_json);
    BESDEBUG(MODULE, prolog << "Found " << attr_count << " value(s) for the history_json attribute." << endl);
}

/**
 * @brief Updates the provenance related "history" attributes in the DDS.
 *
 * @param dds The DDS to update
 * @param ce The constraint expression associated with the request.
 */
void updateHistoryAttributes(DDS *dds, const string &ce) {
    string request_url = dds->filename();
    // remove path info
    request_url = request_url.substr(request_url.find_last_of('/') + 1);
    // remove 'uncompress' cache mangling
    request_url = request_url.substr(request_url.find_last_of('#') + 1);
    if (!ce.empty()) request_url += "?" + ce;

    // Add the new entry to the "history" attribute
    // Get the top level Attribute table.
    AttrTable &globals = dds->get_attr_table();

    // Since many files support "CF" conventions the history tag may already exist
    // in the source dataset, and the code should add an entry to it if possible.

    // Used to indicate that we located a toplevel AttrTable whose name ends in
    // "_GLOBAL" and that has an existing "history" attribute.
    bool added_history = false;

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
                update_cf_history_attr(global_attr_tbl, request_url);
#if !HISTORY_JSON_DIRECT_TO_NETCDF
                // if we do not plan on writing the attribute directly using the netcdf API
                // put the JSON in as a DAP attribute. jhrg 2/28/22
                update_history_json_attr(global_attr_tbl, request_url);
#endif
                added_history = true;
                BESDEBUG(MODULE, prolog << "Added history entries to " << attr_name << endl);
            }
        }

        // if we didn't find a "_GLOBAL" container, we add both the CF History and
        // JSON History attributes to a new "DAP_GLOBAL" attrribute container.
        // We use the function update...() but those make a new attribute if one
        // does not exist and, since this is a new container, they don;t alreay exist.
        // jhrg 2/25/22
        if (!added_history) {
            auto dap_global_at = globals.append_container("DAP_GLOBAL");
            dap_global_at->set_name("DAP_GLOBAL");
            dap_global_at->set_is_global_attribute(true);

            update_cf_history_attr(dap_global_at, request_url);
#if !HISTORY_JSON_DIRECT_TO_NETCDF
            update_history_json_attr(dap_global_at, request_url);
#endif
            BESDEBUG(MODULE, prolog << "No top level AttributeTable name matched '*_GLOBAL'. "
                                       "Created DAP_GLOBAL AttributeTable and added history attributes to it." << endl);
        }
    }
}

/**
 * @brief Updates the provenance related "history" attributes in the DMR.
 *
 * @param dmr The DMR to modify
 * @param ce The constraint expression associated with the request.
 */
void updateHistoryAttributes(DMR *dmr, const string &ce) {
    string request_url = dmr->filename();
    // remove path info
    request_url = request_url.substr(request_url.find_last_of('/') + 1);
    // remove 'uncompress' cache mangling
    request_url = request_url.substr(request_url.find_last_of('#') + 1);
    if (!ce.empty()) request_url += "?" + ce;

    bool added_cf_history = false;
    bool added_json_history = false;
    D4Group *root_grp = dmr->root();
    D4Attributes *root_attrs = root_grp->attributes();
    for (auto attrs = root_attrs->attribute_begin(); attrs != root_attrs->attribute_end(); ++attrs) {
        string name = (*attrs)->name();
        BESDEBUG(MODULE, prolog << "Attribute name is " << name << endl);
        if ((*attrs)->type() == D4AttributeType::attr_container_c && BESUtil::endsWith(name, "_GLOBAL")) {
            // Update Climate Forecast history attribute.
            update_cf_history_attr(*attrs, request_url);
            added_cf_history = true;

            // Update NASA's history_json attribute
#if !HISTORY_JSON_DIRECT_TO_NETCDF
            update_history_json_attr(*attrs, request_url);
            added_json_history = true;
#endif
        }
        else if (name == CF_HISTORY_KEY) { // A top level cf history attribute
            update_cf_history_attr(*attrs, request_url);
            added_cf_history = true;
        }
#if !HISTORY_JSON_DIRECT_TO_NETCDF
        else if (name == HISTORY_JSON_KEY) { // A top level history_json attribute
            update_cf_history_attr(*attrs, request_url);
            added_json_history = true;
        }
#endif
    }
    if (!added_cf_history || !added_json_history) {
        auto *dap_global = new D4Attribute("DAP_GLOBAL", attr_container_c);
        root_attrs->add_attribute_nocopy(dap_global);
        // CF history attribute
        if (!added_cf_history) {
            update_cf_history_attr(dap_global, request_url);
        }
        // NASA's history_json attribute
#if !HISTORY_JSON_DIRECT_TO_NETCDF
        if (!added_json_history) {
            update_history_json_attr(dap_global, request_url);
        }
#endif
    }
}

} // namespace fnoc_history_util