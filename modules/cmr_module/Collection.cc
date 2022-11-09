//
// Created by ndp on 10/31/22.
//
#include "config.h"
#include <cstdio>
#include <sstream>

#include "nlohmann/json.hpp"

#include "BESDebug.h"
#include "CmrNames.h"
#include "Collection.h"

using json = nlohmann::json;
using namespace std;

namespace cmr {

std::string Collection::id() {
    return d_collection_json_obj[CMR_META_KEY][CMR_CONCEPT_ID_KEY].get<std::string>();
}

std::string Collection::abstract() {
    return d_collection_json_obj[CMR_UMM_KEY][CMR_COLLECTION_ABSTRACT_KEY].get<std::string>();
}

std::string Collection::entry_title() {
    return d_collection_json_obj[CMR_UMM_KEY][CMR_COLLECTION_ENTRY_TITLE_KEY].get<std::string>();
}

std::string Collection::short_name() {
    return d_collection_json_obj[CMR_UMM_KEY][CMR_COLLECTION_SHORT_NAME_KEY].get<std::string>();
}

string Collection::to_string() {
    stringstream msg;
    msg << "# # # # # # # # # # # # # # # # # # " << endl;
    msg << "# Collection" << endl;
    msg << "#  concept-id: " << id() << endl;
    msg << "#  short_name: " << short_name() << endl;
    msg << "# entry_title: " << entry_title() << endl;
    msg << "#    abstract: " << abstract() << endl;
    msg << "#" << endl;
    return msg.str();
}

} // cmr