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
#include <stdlib.h>     /* atol */
#include <ctype.h> /* isalpha and isdigit */

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
void HttpdDirScraper::createHttpdDirectoryPageMap(std::string url, std::set<std::string> &pageNodes, std::set<std::string> &pageLeaves) const

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
    string tdOpenStr = "<td ";
    string tdCloseStr = "</td>";

    BESRegex hrefExcludeRegex("(^#.*$)|(^\\?C.*$)|(redirect\\/)|(^\\/$)|(^<img.*$)");
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

                string time_str;
                int start_pos =  getNextElementText(pageStr, "td", aCloseIndex + aCloseStr.length(), time_str);
                BESDEBUG(MODULE, prolog << "time_str: '" << time_str << "'" << endl);

                string size_str;
                start_pos =  getNextElementText(pageStr, "td", start_pos, size_str);
                BESDEBUG(MODULE, prolog << "size_str: '" << size_str << "'" << endl);


                if ((linkText.find("<img") != string::npos) || !(linkText.length()) || (linkText.find("<<<") != string::npos)
                    || (linkText.find(">>>") != string::npos)) {
                    BESDEBUG(MODULE, prolog << "SKIPPING(image|copy|<<<|>>>): " << aElemStr << endl);
                }
                else {
                    if (href.length() == 0 || (((href.find("http://") == 0) || (href.find("https://") == 0)) && !(href.find(url) == 0))) {
                        // SKIPPING
                        BESDEBUG(MODULE, prolog << "SKIPPING(null or remote): " << href << endl);
                    }
                    else if (hrefExcludeRegex.match(href.c_str(), href.length(), 0) > 0) { /// USE MATCH
                        // SKIPPING
                        BESDEBUG(MODULE, prolog << "SKIPPING(hrefExcludeRegex) - href: '" << href << "'"<< endl);
                    }
                    else if (nameExcludeRegex.match(linkText.c_str(), linkText.length(), 0) > 0) { /// USE MATCH
                        // SKIPPING
                        BESDEBUG(MODULE, prolog << "SKIPPING(nameExcludeRegex) - name: '" << linkText << "'" << endl);
                    }
                    else if (BESUtil::endsWith(href, "/")) {
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


/*
 *
 *
 */
long get_size_val(string size_str){

    char scale_c = *size_str.rbegin();
    long scale = 1;

    switch (scale_c){
    case 'K':
        scale = 1e3;
        break;
    case 'M':
        scale = 1e6;
        break;
    case 'G':
        scale = 1e9;
        break;
    case 'T':
        scale = 1e12;
        break;
    case 'P':
        scale = 1e15;
        break;
    default:
        scale = 1;
        break;
    }
    BESDEBUG(MODULE, prolog << "scale: " << scale << endl);

    if(isalpha(scale_c))
        size_str = size_str.substr(0,size_str.length()-1);

    long size = atol(size_str.c_str());
    BESDEBUG(MODULE, prolog << "raw size: " << size << endl);

    size *= scale;
    BESDEBUG(MODULE, prolog << "scaled size: " << size << endl);
    return size;
}


void HttpdDirScraper::createHttpdDirectoryPageMap(std::string url, std::set<bes::CatalogItem *> &items) const
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
    string tdOpenStr = "<td ";
    string tdCloseStr = "</td>";

    BESRegex hrefExcludeRegex("(^#.*$)|(^\\?C.*$)|(redirect\\/)|(^\\/$)|(^<img.*$)");
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

                string time_str;
                int start_pos =  getNextElementText(pageStr, "td", aCloseIndex + aCloseStr.length(), time_str);
                BESDEBUG(MODULE, prolog << "time_str: '" << time_str << "'" << endl);

                string size_str;
                start_pos =  getNextElementText(pageStr, "td", start_pos, size_str);
                BESDEBUG(MODULE, prolog << "size_str: '" << size_str << "'" << endl);


                if ((linkText.find("<img") != string::npos) || !(linkText.length()) || (linkText.find("<<<") != string::npos)
                    || (linkText.find(">>>") != string::npos)) {
                    BESDEBUG(MODULE, prolog << "SKIPPING(image|copy|<<<|>>>): " << aElemStr << endl);
                }
                else {
                    if (href.length() == 0 || (((href.find("http://") == 0) || (href.find("https://") == 0)) && !(href.find(url) == 0))) {
                        // SKIPPING
                        BESDEBUG(MODULE, prolog << "SKIPPING(null or remote): " << href << endl);
                    }
                    else if (hrefExcludeRegex.match(href.c_str(), href.length(), 0) > 0) { /// USE MATCH
                        // SKIPPING
                        BESDEBUG(MODULE, prolog << "SKIPPING(hrefExcludeRegex) - href: '" << href << "'"<< endl);
                    }
                    else if (nameExcludeRegex.match(linkText.c_str(), linkText.length(), 0) > 0) { /// USE MATCH
                        // SKIPPING
                        BESDEBUG(MODULE, prolog << "SKIPPING(nameExcludeRegex) - name: '" << linkText << "'" << endl);
                    }
                    else if (BESUtil::endsWith(href, "/")) {

                        string node_name = href.substr(0,href.length()-1);

                        // it's a directory aka a node
                        BESDEBUG(MODULE, prolog << "NODE: " << node_name << endl);

                        bes::CatalogItem *childNode = new bes::CatalogItem();
                        childNode->set_type(CatalogItem::node);

                        childNode->set_name(node_name);
                        childNode->set_is_data(false);

                        // FIXME: Figure out the LMT if we can... HEAD?
                        childNode->set_lmt(time_str);

                        // FIXME: For nodes the size should be the number of children, but how without crawling?
                        long size = get_size_val(size_str);
                        childNode->set_size(size);

                        items.insert(childNode);

                    }
                    else {
                        // It's a file aka a leaf
                        BESDEBUG(MODULE, prolog << "LEAF: " << href << endl);
                        CatalogItem *leafItem = new CatalogItem();
                        leafItem->set_type(CatalogItem::leaf);
                        leafItem->set_name(href);

                        // FIXME: wrangle up the Typematch and see if we think this thing is data or not.
                        leafItem->set_is_data(false);

                        // FIXME: Find the Last Modified date?
                        leafItem->set_lmt(time_str);

                        // FIXME: Determine size of this thing? Do we "HEAD" all the leaves?
                        long size = get_size_val(size_str);
                        leafItem->set_size(size);

                        items.insert(leafItem);
                    }
                }
            }
            next_start = aCloseIndex + aCloseStr.length();
        }
    }
}

/**
 * @return
 */
int HttpdDirScraper::getNextElementText(const string &page_str, string element_name, int startIndex, string &resultText, bool trim) const
{
    string e_open_str = "<"+element_name+" ";
    string e_close_str = "</"+element_name+">";

    // Locate the next "element_name"  element
    int start = page_str.find(e_open_str, startIndex);
    int end = page_str.find(e_close_str, start + e_open_str.length());
    int length = end + e_close_str.length() - start;
    string element_str = page_str.substr(start, length);

    // Find the text
    start = element_str.find(">") + 1;
    end = element_str.find("<", start);
    length = end - start;
    resultText = element_str.substr(start, length);

    if(trim)
        BESUtil::removeLeadingAndTrailingBlanks(resultText);

    BESDEBUG(MODULE, prolog << "resultText: '" << resultText << "'" << endl);

    return startIndex + element_str.length();
}

bes::CatalogNode *HttpdDirScraper::get_node(const string &url, const string &path) const
{
    BESDEBUG(MODULE, prolog << "Processing url: '" << url << "'"<< endl);
    bes::CatalogNode *node = new bes::CatalogNode(path);

    if (BESUtil::endsWith(url, "/")) {

        set<bes::CatalogItem *> items;
        createHttpdDirectoryPageMap(url, items);

        BESDEBUG(MODULE, prolog << "Found " << items.size() << " items." << endl);

        set<bes::CatalogItem *>::iterator it;

        it = items.begin();
        while (it != items.end()) {
            bes::CatalogItem *item = *it;
            BESDEBUG(MODULE, prolog << "Adding item: '" << item->get_name() << "'"<< endl);

            if(item->get_type() == bes::CatalogItem::item_type::node )
                node->add_node(item);
            else
                node->add_leaf(item);
            it++;
        }

    }
    else {
        std::vector<std::string> url_parts = BESUtil::split(url,'/',true);
        string leaf_name = url_parts.back();

        CatalogItem *item = new CatalogItem();
        item->set_type(CatalogItem::leaf);
        item->set_name(leaf_name);
        // FIXME: Find the Last Modified date?
        item->set_lmt(BESUtil::get_time(true));

        // FIXME: Determine size of this thing? Do we "HEAD" all the leaves?
        item->set_size(1);

        node->set_leaf(item);


    }
    return node;

}

#if 0

bes::CatalogNode *HttpdDirScraper::get_node(const string &url, const string &path) const
{
    BESDEBUG(MODULE, prolog << "Processing url: '" << url << "'"<< endl);
    bes::CatalogNode *node = new bes::CatalogNode(path);

    if (BESUtil::endsWith(url, "/")) {

        set<string> pageNodes;
        set<string> pageLeaves;
        createHttpdDirectoryPageMap(url, pageNodes, pageLeaves);

        BESDEBUG(MODULE, prolog << "Found " << pageNodes.size() << " nodes." << endl);
        BESDEBUG(MODULE, prolog << "Found " << pageLeaves.size() << " leaves." << endl);

        set<string>::iterator it;

        it = pageNodes.begin();
        while (it != pageNodes.end()) {
            string pageNode = *it;
            if (BESUtil::endsWith(pageNode, "/")) pageNode = pageNode.substr(0, pageNode.length() - 1);

            bes::CatalogItem *childNode = new bes::CatalogItem();
            childNode->set_type(CatalogItem::node);

            childNode->set_name(pageNode);
            childNode->set_is_data(false);

            // FIXME: Figure out the LMT if we can... HEAD?
            childNode->set_lmt(BESUtil::get_time(true));

            // FIXME: For nodes the size should be the number of children, but how without crawling?
            childNode->set_size(0);

            node->add_node(childNode);
            it++;
        }

        it = pageLeaves.begin();
        while (it != pageLeaves.end()) {
            string leaf = *it;
            CatalogItem *leafItem = new CatalogItem();
            leafItem->set_type(CatalogItem::leaf);
            leafItem->set_name(leaf);

            // FIXME: wrangle up the Typematch and see if we think this thing is data or not.
            leafItem->set_is_data(false);

            // FIXME: Find the Last Modified date?
            leafItem->set_lmt(BESUtil::get_time(true));

            // FIXME: Determine size of this thing? Do we "HEAD" all the leaves?
            leafItem->set_size(1);

            node->add_leaf(leafItem);
            it++;
        }
    }
    else {
        std::vector<std::string> url_parts = BESUtil::split(url,'/',true);
        string leaf_name = url_parts.back();

        CatalogItem *item = new CatalogItem();
        item->set_type(CatalogItem::leaf);
        item->set_name(leaf_name);
        // FIXME: Find the Last Modified date?
        item->set_lmt(BESUtil::get_time(true));

        // FIXME: Determine size of this thing? Do we "HEAD" all the leaves?
        item->set_size(1);

        node->set_leaf(item);


    }
    return node;

}
#endif


} // namespace httpd_catalog

