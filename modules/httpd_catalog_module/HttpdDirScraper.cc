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

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>     /* atol */
#include <ctype.h> /* isalpha and isdigit */
#include <time.h> /* mktime */

#include <BESDebug.h>
#include <BESUtil.h>
#include <BESRegex.h>
#include <BESCatalogList.h>
#include <BESCatalogUtils.h>
#include <CatalogItem.h>

#include "RemoteResource.h"
#include "HttpdCatalogNames.h"

#include "HttpdDirScraper.h"

using namespace std;
using bes::CatalogItem;

#define prolog std::string("HttpdDirScraper::").append(__func__).append("() - ")

namespace httpd_catalog {

HttpdDirScraper::HttpdDirScraper()
{
    // There was probably a better way to make this association but this worked.
    d_months.insert(pair<string, int>(string("jan"), 0));
    d_months.insert(pair<string, int>(string("feb"), 1));
    d_months.insert(pair<string, int>(string("mar"), 2));
    d_months.insert(pair<string, int>(string("apr"), 3));
    d_months.insert(pair<string, int>(string("may"), 4));
    d_months.insert(pair<string, int>(string("jun"), 5));
    d_months.insert(pair<string, int>(string("jul"), 6));
    d_months.insert(pair<string, int>(string("aug"), 7));
    d_months.insert(pair<string, int>(string("sep"), 8));
    d_months.insert(pair<string, int>(string("oct"), 9));
    d_months.insert(pair<string, int>(string("nov"), 10));
    d_months.insert(pair<string, int>(string("dec"), 11));
}

/*
 * @brief Converts an Apache httpd directory page "size" string (23K, 45M, 32G, etc)
 * to an actual value, approximate though it may be.
 */
long HttpdDirScraper::get_size_val(const string size_str) const
{
    if(size_str.empty())
        return 0;
    
    char scale_c = *size_str.rbegin();
    long scale = 1;

    switch (scale_c) {
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

    string result = size_str;
    if (isalpha(scale_c)) result = size_str.substr(0, size_str.size() - 1);

    long size = atol(result.c_str());
    BESDEBUG(MODULE, prolog << "raw size: " << size << endl);

    size *= scale;
    BESDEBUG(MODULE, prolog << "scaled size: " << size << endl);
    return size;
}

/**
 * @ brief Make a string of a tm struct (time structure) value;
 */
string show_tm_struct(const tm tms)
{
    stringstream ss;
    ss << "tm_sec:   " << tms.tm_sec << endl;
    ss << "tm_min:   " << tms.tm_min << endl;
    ss << "tm_hour:  " << tms.tm_hour << endl;
    ss << "tm_mday:  " << tms.tm_mday << endl;
    ss << "tm_mon:   " << tms.tm_mon << endl;
    ss << "tm_year:  " << tms.tm_year << endl;
    ss << "tm_wday:  " << tms.tm_wday << endl;
    ss << "tm_yday:  " << tms.tm_yday << endl;
    ss << "tm_isdst: " << tms.tm_isdst << endl;
    return ss.str();
}

/**
 * @brief Zero a tm struct
 */
void zero_tm_struct(tm &tms)
{
    tms.tm_sec = 0;
    tms.tm_min = 0;
    tms.tm_hour = 0;
    tms.tm_mday = 1;
    tms.tm_mon = 0;
    tms.tm_year = 0;
    tms.tm_wday = 0;
    tms.tm_yday = 0;
    tms.tm_isdst = 0;
}


string HttpdDirScraper::httpd_time_to_iso_8601(const string httpd_time) const
{
    if(httpd_time.empty())
        return httpd_time;

    vector<string> tokens;
    string delimiters = "- :";
    BESUtil::tokenize(httpd_time, tokens, delimiters);

    BESDEBUG(MODULE, prolog << "Found " << tokens.size() << " tokens." << endl);
    vector<string>::iterator it = tokens.begin();
    int i = 0;
    if (BESDebug::IsSet(MODULE)) {
        while (it != tokens.end()) {
            BESDEBUG(MODULE, prolog << "    token["<< i++ << "]: "<< *it << endl);
            it++;
        }
    }

    BESDEBUG(MODULE, prolog << "Second Field: "<< tokens[1] << endl);

    const char *second_field = tokens[1].c_str();
    bool is_alpha = true;
    for(unsigned long i=0; is_alpha && i< tokens[1].size(); i++){
        is_alpha = isalpha(second_field[i]);
    }
    time_t theTime;
    if(is_alpha){
        BESDEBUG(MODULE, prolog << "Detected Time Format A (\"DD-MM-YYY hh:mm\")" << endl);
        theTime = parse_time_format_A(tokens);
    }
    else {
        BESDEBUG(MODULE, prolog << "Detected Time Format B (\"YYYY-MM-DD hh:mm\")" << endl);
        theTime = parse_time_format_B(tokens);
    }
    return BESUtil::get_time(theTime, false);

}

/**
 * Apache httpd directories utilize a time format of
 *  "DD-MM-YYY hh:mm" example: "19-Oct-2018 19:32"
 *  here we assume the time zone is UTC and off we go.
 */
time_t HttpdDirScraper::parse_time_format_A(const vector<string> tokens) const
{
    // void BESUtil::tokenize(const string& str, vector<string>& tokens, const string& delimiters)
    struct tm tm{};
    // jhrg 2/2/24 zero_tm_struct(tm);

    if (tokens.size() > 2) {
        std::istringstream(tokens[0]) >> tm.tm_mday;
        BESDEBUG(MODULE, prolog << "    tm.tm_mday: "<< tm.tm_mday << endl);

        pair<string, int> mnth = *d_months.find(BESUtil::lowercase(tokens[1]));
        BESDEBUG(MODULE, prolog << "    mnth.first: "<< mnth.first << endl);
        BESDEBUG(MODULE, prolog << "    mnth.second: "<< mnth.second << endl);
        tm.tm_mon = mnth.second;
        BESDEBUG(MODULE, prolog << "    tm.tm_mon: "<< tm.tm_mon << endl);

        std::istringstream(tokens[2]) >> tm.tm_year;
        tm.tm_year -= 1900;
        BESDEBUG(MODULE, prolog << "    tm.tm_year: "<< tm.tm_year << endl);

        if (tokens.size() > 4) {
            std::istringstream(tokens[3]) >> tm.tm_hour;
            BESDEBUG(MODULE, prolog << "    tm.tm_hour: "<< tm.tm_hour << endl);
            std::istringstream(tokens[4]) >> tm.tm_min;
            BESDEBUG(MODULE, prolog << "    tm.tm_min: "<< tm.tm_min << endl);
        }
    }

    BESDEBUG(MODULE, prolog << "tm struct: " << endl << show_tm_struct(tm));

    time_t theTime = mktime(&tm);
    BESDEBUG(MODULE, prolog << "theTime: " << theTime << endl);
    return theTime;
}

/**
 * Newer (??) Apache httpd directories utilize a time format of
 *  "YYYY-MM-DD hh:mm" example: "2012-01-02 10:03"
 *  here we assume the time zone is UTC and off we go.
 */
time_t HttpdDirScraper::parse_time_format_B(const vector<string> tokens) const
{
    // void BESUtil::tokenize(const string& str, vector<string>& tokens, const string& delimiters)
    struct tm tm{};
    if (tokens.size() > 2) {
        std::istringstream(tokens[0]) >> tm.tm_year;
        tm.tm_year -= 1900;
        BESDEBUG(MODULE, prolog << "    tm.tm_year: "<< tm.tm_year << endl);

        std::istringstream(tokens[1]) >> tm.tm_mon;
        BESDEBUG(MODULE, prolog << "    tm.tm_mon: "<< tm.tm_mon << endl);

        std::istringstream(tokens[2]) >> tm.tm_mday;
        BESDEBUG(MODULE, prolog << "    tm.tm_mday: "<< tm.tm_mday << endl);

        if (tokens.size() > 4) {
            std::istringstream(tokens[3]) >> tm.tm_hour;
            BESDEBUG(MODULE, prolog << "    tm.tm_hour: "<< tm.tm_hour << endl);
            std::istringstream(tokens[4]) >> tm.tm_min;
            BESDEBUG(MODULE, prolog << "    tm.tm_min: "<< tm.tm_min << endl);
        }
    }

    BESDEBUG(MODULE, prolog << "tm struct: " << endl << show_tm_struct(tm));

    time_t theTime = mktime(&tm);
    BESDEBUG(MODULE, prolog << "ISO-8601 Time: " << theTime << endl);
    return theTime;
}

/**
 * @brief Converts an Apache httpd directory page into a collection of bes::CatalogItems.
 *
 * If one considers each Apache httpd generated directory page to be equivalent to
 * a bes::CatalogNode then this method examines the contents of the httpd directory page and
 * builds child node and leaf bes:CatalogItems based on what it finds.
 *
 * isData: The besCatalogItem objects that are leaves are evaluated against the BES_DEFAULT_CATALOG
 * TypeMatch by retrieving the BES_DEFAULT_CATALOG's BESCatalogUtils and then calling
 * BESCatalogUtils::is_data(leaf_name) This why this catalog does not utilize it's own
 * TypeMatch string.
 *
 * @param url The url of the source httpd directory.
 * @param items The map (list sorted by href) of catalog Items generated by the scrape processes. The map's
 * key is the bes::CatalogItem::name().
 */
void HttpdDirScraper::createHttpdDirectoryPageMap(std::string url, std::map<std::string, bes::CatalogItem *> &items) const
{
    const BESCatalogUtils *cat_utils = BESCatalogList::TheCatalogList()->find_catalog(BES_DEFAULT_CATALOG)->get_catalog_utils();

    // Go get the text from the remote resource
    std::shared_ptr<http::url> url_ptr(new http::url(url));
    http::RemoteResource rhr(url_ptr);
    rhr.retrieve_resource();
    stringstream buffer;

    ifstream cache_file_is(rhr.get_filename().c_str());
    if(!cache_file_is.is_open()){
        string msg = prolog + "ERROR - Failed to open cache file: " + rhr.get_filename();
        BESDEBUG(MODULE, msg << endl);
        throw BESInternalError(msg ,__FILE__, __LINE__ );
    }

    buffer << cache_file_is.rdbuf();
    string pageStr = buffer.str();
    BESDEBUG(MODULE, prolog << "Page Content: " << endl << pageStr << endl);

    // Does it look like an Apache httpd Index listing?
    if(pageStr.find("<title>Index of ") == string::npos){
        // Nope. Time to leave.
        BESDEBUG(MODULE, prolog << "The url: " << url << " does not appear to reference an Apache httpd Index page." << endl);
        return;
    }

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
            int aCloseIndex = pageStr.find(aCloseStr, aOpenIndex + aOpenStr.size());
            if (aCloseIndex < 0) {
                done = true;
            }
            else {
                int length;

                // Locate the entire <a /> element
                BESDEBUG(MODULE, prolog << "aOpenIndex: " << aOpenIndex << endl);
                BESDEBUG(MODULE, prolog << "aCloseIndex: " << aCloseIndex << endl);
                length = aCloseIndex + aCloseStr.size() - aOpenIndex;
                string aElemStr = pageStr.substr(aOpenIndex, length);
                BESDEBUG(MODULE, prolog << "Processing link: " << aElemStr << endl);

                // Find the link text
                int start = aElemStr.find(">") + 1;
                int end = aElemStr.find("<", start);
                length = end - start;
                string linkText = aElemStr.substr(start, length);
                BESDEBUG(MODULE, prolog << "Link Text: " << linkText << endl);

                // Locate the href attribute
                start = aElemStr.find(hrefStr) + hrefStr.size();
                end = aElemStr.find("\"", start);
                length = end - start;
                string href = aElemStr.substr(start, length);
                BESDEBUG(MODULE, prolog << "href: " << href << endl);

                // attempt to get time string
                string time_str;
                int start_pos = getNextElementText(pageStr, "td", aCloseIndex + aCloseStr.size(), time_str);
                BESDEBUG(MODULE, prolog << "time_str: '" << time_str << "'" << endl);

                // attempt to get size string
                string size_str;
                start_pos = getNextElementText(pageStr, "td", start_pos, size_str);
                BESDEBUG(MODULE, prolog << "size_str: '" << size_str << "'" << endl);

                if ((linkText.find("<img") != string::npos) || !(linkText.size()) || (linkText.find("<<<") != string::npos)
                    || (linkText.find(">>>") != string::npos)) {
                    BESDEBUG(MODULE, prolog << "SKIPPING(image|copy|<<<|>>>): " << aElemStr << endl);
                }
                else {
                    if (href.size() == 0 || (((href.find("http://") == 0) || (href.find("https://") == 0)) && !(href.find(url) == 0))) {
                        // SKIPPING
                        BESDEBUG(MODULE, prolog << "SKIPPING(null or remote): " << href << endl);
                    }
                    else if (hrefExcludeRegex.match(href.c_str(), href.size(), 0) > 0) {
                        // SKIPPING
                        BESDEBUG(MODULE, prolog << "SKIPPING(hrefExcludeRegex) - href: '" << href << "'"<< endl);
                    }
                    else if (nameExcludeRegex.match(linkText.c_str(), linkText.size(), 0) > 0) {
                        // SKIPPING
                        BESDEBUG(MODULE, prolog << "SKIPPING(nameExcludeRegex) - name: '" << linkText << "'" << endl);
                    }
                    else if (BESUtil::endsWith(href, "/")) {
                        string node_name = href.substr(0, href.size() - 1);
                        // it's a directory aka a node
                        BESDEBUG(MODULE, prolog << "NODE: " << node_name << endl);
                        bes::CatalogItem *childNode = new bes::CatalogItem();
                        childNode->set_type(CatalogItem::node);
                        childNode->set_name(node_name);
                        childNode->set_is_data(false);
                        string iso_8601_time = httpd_time_to_iso_8601(time_str);
                        childNode->set_lmt(iso_8601_time);
                        // FIXME: For nodes the size should be the number of children, but how without crawling?
                        long size = get_size_val(size_str);
                        childNode->set_size(size);

                        items.insert(pair<std::string, bes::CatalogItem *>(node_name, childNode));
                    }
                    else {
                        // It's a file aka a leaf
                        BESDEBUG(MODULE, prolog << "LEAF: " << href << endl);
                        CatalogItem *leafItem = new CatalogItem();
                        leafItem->set_type(CatalogItem::leaf);
                        leafItem->set_name(href);
                        leafItem->set_is_data(cat_utils->is_data(href));
                        string iso_8601_time = httpd_time_to_iso_8601(time_str);
                        leafItem->set_lmt(iso_8601_time);
                        long size = get_size_val(size_str);
                        leafItem->set_size(size);

                        items.insert(pair<std::string, bes::CatalogItem *>(href, leafItem));
                    }
                }
            }
            next_start = aCloseIndex + aCloseStr.size();
        }
    }
}

/**
 * Get's the text content of the next element_name element beginning at startIndex.
 * The correctness of this is very limitied and it works just well enough to grab the td elements that
 * hold last modified time and size in the httpd directory content. Just sayin.
 *
 * @param page_str The string to examine.
 * @param element_name the name of the element whose text to get.
 * @param startIndex The index in page_str at which to begin the search.
 * @param resultText The text content of the element.
 * @param trim If true then leading and trailing whitespace will be removed from the result string. default: true
 * @return The next start index, after the td element closer.
 */
int HttpdDirScraper::getNextElementText(const string &page_str, const string element_name, int startIndex, string &resultText, bool trim) const
{
    string e_open_str = "<" + element_name + " ";
    string e_close_str = "</" + element_name + ">";

    // Locate the next "element_name"  element
    int start = page_str.find(e_open_str, startIndex);
    int end = page_str.find(e_close_str, start + e_open_str.size());
    if(start<0 || end<0 || end<start){
        resultText="";
        return startIndex;
    }

    int length = end + e_close_str.size() - start;
    string element_str = page_str.substr(start, length);

    // Find the text
    start = element_str.find(">") + 1;
    end = element_str.find("<", start);
    length = end - start;
    resultText = element_str.substr(start, length);

    if (trim) BESUtil::removeLeadingAndTrailingBlanks(resultText);

    BESDEBUG(MODULE, prolog << "resultText: '" << resultText << "'" << endl);
    return startIndex + element_str.size();
}

/*
 * @brief Returns the catalog node represented by the httpd directory page returned
 * by dereferencing the passed url.
 * @param url The url of the Apache httpd directory to process.
 * @param path The path prefix that associates the location of this generated CatalogNode with it's
 * correct position in the local service path.
 */
bes::CatalogNode *HttpdDirScraper::get_node(const string &url, const string &path) const
{
    BESDEBUG(MODULE, prolog << "Processing url: '" << url << "'"<< endl);
    bes::CatalogNode *node = new bes::CatalogNode(path);

    if (BESUtil::endsWith(url, "/")) {
        // This always means the URL points to a node when coming from httpd
        map<string, bes::CatalogItem *> items;
        createHttpdDirectoryPageMap(url, items);

        BESDEBUG(MODULE, prolog << "Found " << items.size() << " items." << endl);
        map<string, bes::CatalogItem *>::iterator it;
        it = items.begin();
        while (it != items.end()) {
            bes::CatalogItem *item = it->second;
            BESDEBUG(MODULE, prolog << "Adding item: '" << item->get_name() << "'"<< endl);
            if (item->get_type() == CatalogItem::node)
                node->add_node(item);
            else
                node->add_leaf(item);
            it++;
        }
    }
    else {
        // It's a leaf aka "item" response.
        const BESCatalogUtils *cat_utils = BESCatalogList::TheCatalogList()->find_catalog(BES_DEFAULT_CATALOG)->get_catalog_utils();
        std::vector<std::string> url_parts = BESUtil::split(url, '/', true);
        string leaf_name = url_parts.back();

        CatalogItem *item = new CatalogItem();
        item->set_type(CatalogItem::leaf);
        item->set_name(leaf_name);
        item->set_is_data(cat_utils->is_data(leaf_name));

        // FIXME: Find the Last Modified date? Head??
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
            if (BESUtil::endsWith(pageNode, "/")) pageNode = pageNode.substr(0, pageNode.size() - 1);

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

}
 // namespace httpd_catalog

