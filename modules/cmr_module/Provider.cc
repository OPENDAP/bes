//
// Created by ndp on 10/31/22.
//

#include <string>
#include "rjson_utils.h"

#include "Provider.h"

using namespace std;

namespace cmr {
string provider_ID = "provider_id";


Provider::Provider(const rapidjson::Value& provider_obj){
    set_id(provider_obj);
}

void Provider::set_id(const rapidjson::Value& po){
    rjson_utils rju;
    this->d_id = rju.getStringValue(po, provider_ID);
}






} // cmr