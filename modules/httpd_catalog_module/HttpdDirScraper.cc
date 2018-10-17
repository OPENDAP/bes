/*
 * HttpdDirScraper.cc
 *
 *  Created on: Oct 15, 2018
 *      Author: ndp
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

#include <BESDebug.h>
#include <BESUtil.h>
#include <BESRegex.h>
#include <CatalogItem.h>

#include "RemoteHttpResource.h"
#include "HttpdCatalogNames.h"

#include "HttpdDirScraper.h"

using namespace std;
using bes::CatalogItem;

#define prolog std::string("HttpdDirScraper::").append(__func__).append("() - ")


namespace httpd_catalog {


HttpdDirScraper::HttpdDirScraper()
{
    // TODO Auto-generated constructor stub

}

HttpdDirScraper::~HttpdDirScraper()
{
    // TODO Auto-generated destructor stub
}

void
HttpdDirScraper::createHttpdDirectoryPageMap(std::string url, std::set<std::string> pageNodes, std::set<std::string> pageLeaves/*, PathPrefix*/) const
{
    // Go get the text from the remote resource
    RemoteHttpResource rhr(url);
    rhr.retrieveResource();
    ifstream t(rhr.getCacheFileName());
    stringstream buffer;
    buffer << t.rdbuf();
    string pageStr = buffer.str();

    string aOpenStr = "<a ";
    string aCloseStr = "</a>";
    string hrefStr = "href=\"";
    BESRegex hrefExcludeRegex ("(^#.*$)|(^\\?C.*$)|(redirect\\/)|(^\\/$)|(^<img.*$)");
    BESRegex nameExcludeRegex("^Parent Directory$");

    bool done = false;
    int next_start = 0;
    while (!done) {
        int aOpenIndex = pageStr.find(aOpenStr, next_start);
        if (aOpenIndex < 0) {
            done = true;
        }
        else {
            int aCloseIndex = pageStr.find(aCloseStr, aOpenIndex + aOpenStr.length());
            if (aCloseIndex < 0) {
                done = true;
            }
            else {
                BESDEBUG(MODULE, prolog << "aOpenIndex: " << aOpenIndex << endl);
                BESDEBUG(MODULE, prolog << "aCloseIndex: " << aCloseIndex << endl);
                string aElemStr = pageStr.substr(aOpenIndex, aCloseIndex + aCloseStr.length());
                BESDEBUG(MODULE, prolog << "Processing link: " << aElemStr << endl);

                int start = aElemStr.find(">");
                int end = aElemStr.find("<", start);
                string linkText = aElemStr.substr(start, end);
                BESDEBUG(MODULE, prolog << "Link Text: " << linkText << endl);

                start = aElemStr.find(hrefStr) + hrefStr.length();
                end = aElemStr.find("\"", start);
                string href = aElemStr.substr(start, end);
                BESDEBUG(MODULE, prolog << "href: " << href << endl);

                if (!(linkText.find("<img") < 0) || !(linkText.length()) || !(linkText.find("<<<") < 0) || !(linkText.find(">>>") < 0)) {
                    BESDEBUG(MODULE, prolog << "SKIPPING(image|copy|<<<|>>>): " << aElemStr << endl);
                }
                else {
                    if (href.length() == 0 || (((href.find("http") == 0) || (href.find("https") == 0)) && !(href.find(url) == 0))) {
                        // SKIPPING
                        BESDEBUG(MODULE, prolog << "SKIPPING(null or remote): " << href << endl);
                    }
                    else if (hrefExcludeRegex.match(href.c_str(),href.length(),0) || nameExcludeRegex.match(linkText.c_str(),linkText.length(),0)) { /// USE MATCH
                        // SKIPPING
                        BESDEBUG(MODULE, prolog << "SKIPPING(regex) - href: '" << href << "' name: '" << linkText << "'" << endl);
                    }
                    else if (BESUtil::endsWith(url, "/")) {
                        // it's a directory aka a node
                        BESDEBUG(MODULE, prolog << "NODE: " << href << endl);
                        pageNodes.insert(href);
                    }
                    else {
                        // It's a file aka a leaf
                        BESDEBUG(MODULE, prolog << "LEAF: " << href << endl);
                        pageLeaves.insert(href);
                    }
                }
            }
            next_start = aCloseIndex + aCloseStr.length();
        }
    }
}



bes::CatalogNode *HttpdDirScraper::get_node(const string &url, const string &path) const
{
    set<string> pageNodes;
    set<string> pageLeaves;
    createHttpdDirectoryPageMap(url, pageNodes, pageLeaves);
    set<string>::iterator it;
    bes::CatalogNode *node =  new bes::CatalogNode(path);

    it =pageNodes.begin();
    for (; it!=pageNodes.end(); ++it){
        string pageNode = *it;
        bes::CatalogItem *collection = new bes::CatalogItem();
        collection->set_type(CatalogItem::node);
        collection->set_name(pageNode);
        collection->set_is_data(false);

        // FIXME: Figure out the LMT if we can... HEAD?
        collection->set_lmt(BESUtil::get_time(true));

        // FIXME: For nodes the size should be the number of children, but how without crawling?
        collection->set_size(0);

        node->add_node(collection);
    }

    it=pageLeaves.begin();
    for (; it!=pageLeaves.end(); ++it){
        string leaf = *it;
        CatalogItem *granuleItem = new CatalogItem();
        granuleItem->set_type(CatalogItem::leaf);
        granuleItem->set_name(leaf);

        // FIXME: wrangle up the Typematch and see if we think this thing is data or not.
        granuleItem->set_is_data(false);

        // FIXME: Find the Last Modified date?
        granuleItem->set_lmt(BESUtil::get_time(true));

        // FIXME: Determine size of this thing? Do we "HEAD" all the leaves?
        granuleItem->set_size(-1);

        node->set_leaf(granuleItem);
    }

    return node;
}

} // namespace httpd_catalog




