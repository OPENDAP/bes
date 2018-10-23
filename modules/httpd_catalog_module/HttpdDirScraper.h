/*
 * HttpdDirScraper.h
 *
 *  Created on: Oct 15, 2018
 *      Author: ndp
 */

#ifndef MODULES_CMR_MODULE_HTTPDDIRSCRAPER_H_
#define MODULES_CMR_MODULE_HTTPDDIRSCRAPER_H_

#include <set>
#include <string>
#include <CatalogNode.h>
#include <HttpdCatalog.h>

namespace httpd_catalog {

class HttpdDirScraper {
private:
    const HttpdCatalog *d_catalog;

    int getNextElementText(const string &page_str, string element_name, int startIndex, string &resultText, bool trim=true) const;

    void createHttpdDirectoryPageMap(std::string url, std::set<bes::CatalogItem *> &items) const;
    void createHttpdDirectoryPageMap(std::string url, std::set<std::string> &pageNodes, std::set<std::string> &pageLeave) const;

public:
    HttpdDirScraper(const HttpdCatalog *hc);
    virtual ~HttpdDirScraper();

    virtual bes::CatalogNode *get_node(const string &url, const string &path) const;

};
} // namespace httpd_catalog

#endif /* MODULES_CMR_MODULE_HTTPDDIRSCRAPER_H_ */
