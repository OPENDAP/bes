//
// Created by ndp on 10/31/22.
//

#ifndef HYRAX_GIT_PROVIDER_H
#define HYRAX_GIT_PROVIDER_H

#include "config.h"

#include <string>
#include <vector>

namespace cmr {

/*
   {
    "provider": {
      "contacts": [
        {
          "email": "michael.p.morahan@nasa.gov",
          "first_name": "Michael",
          "last_name": "Morahan",
          "phones": [],
          "role": "Default Contact"
        }
      ],
      "description_of_holdings": "EO European and Canadian missions Earth Science Data",
      "discovery_urls": [
        "https://fedeo-client.ceos.org/about/"
      ],
      "id": "E37E931C-A94A-3F3C-8FA1-206EB96B465C",
      "organization_name": "Federated EO missions support environment",
      "provider_id": "FEDEO",
      "provider_types": [
        "CMR"
      ],
      "rest_only": true
    }
 */

/*
    {
        "email": "michael.p.morahan@nasa.gov",
        "first_name": "Michael",
        "last_name": "Morahan",
        "phones": [],
        "role": "Default Contact"
    }

 */
struct contact {
    std::string email;
    std::string first_name;
    std::string last_name;
    std::vector<std::string> phones;
    std::string role;
};

class Provider {
private:
    std::vector<contact> contacts;
    std::string description_of_holdings;
    std::vector<std::string> discovery_urls;
    std::string id;
    std::string organization_name;
    std::string provider_id;
    std::vector<std::string> provider_types;
    bool rest_only;

public:


};

} // cmr

#endif //HYRAX_GIT_PROVIDER_H
