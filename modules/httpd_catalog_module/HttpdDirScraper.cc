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
HttpdDirScraper::createHttpdDirectoryPageMap(std::string url, std::set<std::string> &pageNodes, std::set<std::string> &pageLeaves) const
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
                int length;

                // Locate out the entire <a /> element
                BESDEBUG(MODULE, prolog << "aOpenIndex: " << aOpenIndex << endl);
                BESDEBUG(MODULE, prolog << "aCloseIndex: " << aCloseIndex << endl);
                length = aCloseIndex + aCloseStr.length() - aOpenIndex;
                string aElemStr = pageStr.substr(aOpenIndex, length);
                BESDEBUG(MODULE, prolog << "Processing link: " << aElemStr << endl);

                // Find the link text
                int start = aElemStr.find(">") + 1;
                int end = aElemStr.find("<", start);
                length = end - start;
                string linkText = aElemStr.substr(start, length);
                BESDEBUG(MODULE, prolog << "Link Text: " << linkText << endl);

                // Locate the href attribute
                start = aElemStr.find(hrefStr) + hrefStr.length();
                end = aElemStr.find("\"", start);
                length = end - start;
                string href = aElemStr.substr(start, length);
                BESDEBUG(MODULE, prolog << "href: " << href << endl);

                if ((linkText.find("<img") != string::npos) || !(linkText.length()) || (linkText.find("<<<") != string::npos) || (linkText.find(">>>") != string::npos)) {
                    BESDEBUG(MODULE, prolog << "SKIPPING(image|copy|<<<|>>>): " << aElemStr << endl);
                }
                else {
                    if (href.length() == 0 || (((href.find("http://") == 0) || (href.find("https://") == 0)) && !(href.find(url) == 0))) {
                        // SKIPPING
                        BESDEBUG(MODULE, prolog << "SKIPPING(null or remote): " << href << endl);
                    }
                    else if (hrefExcludeRegex.match(href.c_str(),href.length(),0) > 0) { /// USE MATCH
                        // SKIPPING
                        BESDEBUG(MODULE, prolog << "SKIPPING(hrefExcludeRegex) - href: '" << href << "'"<< endl);
                    }
                    else if (nameExcludeRegex.match(linkText.c_str(),linkText.length(),0) > 0) { /// USE MATCH
                        // SKIPPING
                        BESDEBUG(MODULE, prolog << "SKIPPING(nameExcludeRegex) - name: '" << linkText << "'" << endl);
                    }
                    else if (BESUtil::endsWith(href, "/")) {
                        // it's a directory aka a node
                        BESDEBUG(MODULE, prolog << "NODE: " << href << endl);
                        href = href.substr(0,href.length()-1);
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

    BESDEBUG(MODULE, prolog << "Found " << pageNodes.size() << " nodes." << endl);
    BESDEBUG(MODULE, prolog << "Found " << pageLeaves.size() << " leaves." << endl);

    set<string>::iterator it;
    bes::CatalogNode *node =  new bes::CatalogNode(path);

    it =pageNodes.begin();
    while(it!=pageNodes.end()){
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
        it++;
    }

    it=pageLeaves.begin();
    while (it!=pageLeaves.end()){
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

        node->add_leaf(granuleItem);
        it++;
    }

    return node;
}

} // namespace httpd_catalog




