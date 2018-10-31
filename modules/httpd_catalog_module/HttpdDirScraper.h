// HttpdDirScraper.h
// -*- mode: c++; c-basic-offset:4 -*-
//
// This file is part of httpd_catalog_module, A C++ module that can be loaded in to
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


#ifndef MODULES_CMR_MODULE_HTTPDDIRSCRAPER_H_
#define MODULES_CMR_MODULE_HTTPDDIRSCRAPER_H_

#include <set>
#include <string>
#include <CatalogNode.h>
#include "HttpdCatalog.h"

namespace httpd_catalog {

/**
 * @brief This class knows how to scrape an httpd generated directory page and build a BES CatalogNode response
 * from the result.
 *
 * The scraping is done procedurally. The primary assumption is that links that point to nodes always end
 * in "/". Links that end in other characters are assumed to be links to "leaves".
 *
 */
class HttpdDirScraper {
private:
    map<string,int> d_months;

    int getNextElementText(const string &page_str, string element_name, int startIndex, string &resultText, bool trim=true) const;
    void createHttpdDirectoryPageMap(std::string url, std::map<std::string, bes::CatalogItem *> &items) const;
    long get_size_val(const string size_str) const;
    string httpd_time_to_iso_8601(const string httpd_time) const;

public:
    HttpdDirScraper();
    ~HttpdDirScraper() { }
    virtual bes::CatalogNode *get_node(const string &url, const string &path) const;
};
} // namespace httpd_catalog

#endif /* MODULES_CMR_MODULE_HTTPDDIRSCRAPER_H_ */
