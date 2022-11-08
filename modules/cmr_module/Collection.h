//
// Created by ndp on 10/31/22.
//

#ifndef HYRAX_GIT_COLLECTION_H
#define HYRAX_GIT_COLLECTION_H

#include "config.h"

#include <string>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

namespace cmr {

class Collection {
private:
    nlohmann::json d_collection_json_obj;

public:
    explicit Collection(nlohmann::json collection_obj): d_collection_json_obj(std::move(collection_obj)){}

    std::string id();
    std::string abstract();
    std::string entry_title();
    std::string short_name();

};

} // cmr

#endif //HYRAX_GIT_COLLECTION_H
