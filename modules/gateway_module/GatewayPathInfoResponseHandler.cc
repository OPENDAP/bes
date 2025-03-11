// -*- mode: c++; c-basic-offset:4 -*-
//
// GatewayPathInfoResponseHandler.cc
//
// This file is part of BES dap package
//
// Copyright (c) 2015v OPeNDAP, Inc.
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
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#include "config.h"

#include <sstream>
#include <fstream>
#include <time.h>

#include <cerrno>
#include <cstring>

#include "GatewayPathInfoResponseHandler.h"

#include "BESDebug.h"

#include "BESInfoList.h"
#include "BESInfo.h"
#include "BESUtil.h"
#include "BESRequestHandlerList.h"
#include "BESRequestHandler.h"
#include "BESNames.h"
#include "BESDapNames.h"
#include "BESDataNames.h"
#include "BESCatalogList.h"
#include "BESCatalog.h"
#include "BESCatalogEntry.h"
#include "BESCatalogUtils.h"
#include "BESSyntaxUserError.h"
#include "BESForbiddenError.h"
#include "BESNotFoundError.h"
#include "BESStopWatch.h"

using std::endl;
using std::map;
using std::string;
using std::list;
using std::ostream;

#define PATH_INFO_RESPONSE "PathInfo"
#define PATH "path"
#define VALID_PATH "validPath"
#define REMAINDER  "remainder"
#define IS_DATA "isData"
#define IS_FILE "isFile"
#define IS_DIR  "isDir"
#define IS_ACCESSIBLE "access"
#define SIZE  "size"
#define LMT  "lastModified"

#define prolog std::string("GatewayPathInfoResponseHandler::").append(__func__).append("() - ")

GatewayPathInfoResponseHandler::GatewayPathInfoResponseHandler(const string &name) :
    BESResponseHandler(name), _response(0)
{
}

GatewayPathInfoResponseHandler::~GatewayPathInfoResponseHandler()
{
}

/** @brief executes the command 'show catalog|leaves [for &lt;node&gt;];' by
 * returning nodes or leaves at the top level or at the specified node.
 *
 * The response object BESInfo is created to store the information.
 *
 * @param dhi structure that holds request and response information
 * @see BESDataHandlerInterface
 * @see BESInfo
 * @see BESRequestHandlerList
 */
void GatewayPathInfoResponseHandler::execute(BESDataHandlerInterface &dhi)
{
    BES_STOPWATCH_START(SPI_DEBUG_KEY, prolog + "Timer");

    BESDEBUG(SPI_DEBUG_KEY, prolog << "BEGIN" << endl );

    BESInfo *info = BESInfoList::TheList()->build_info();
    _response = info;

    string container = dhi.data[CONTAINER];
#if 0
    string catname;
    string defcatname = BESCatalogList::TheCatalogList()->default_catalog_name();
#endif

    BESCatalog *defcat = BESCatalogList::TheCatalogList()->default_catalog();
    if (!defcat)
        throw BESInternalError("Not able to find the default catalog.", __FILE__, __LINE__);

    // remove all of the leading slashes from the container name
    string::size_type notslash = container.find_first_not_of("/", 0);
    if (notslash != string::npos) {
        container = container.substr(notslash);
    }

    // see if there is a catalog name here. It's only a possible catalog name
    string catname;
    string::size_type slash = container.find_first_of("/", 0);
    if (slash != string::npos) {
        catname = container.substr(0, slash);
    }
    else {
        catname = container;
    }

    // see if this catalog exists. If it does, then remove the catalog
    // name from the container (node)
    BESCatalog *catobj = BESCatalogList::TheCatalogList()->find_catalog(catname);
    if (catobj) {
        if (slash != string::npos) {
            container = container.substr(slash + 1);

            // remove repeated slashes
            notslash = container.find_first_not_of("/", 0);
            if (notslash != string::npos) {
                container = container.substr(notslash);
            }
        }
        else {
            container = "";
        }
    }

    if (container.empty()) container = "/";

    if (container[0] != '/') container = "/" + container;

    BESDEBUG(SPI_DEBUG_KEY, prolog << "container: " << container << endl );

    info->begin_response(SHOW_GATEWAY_PATH_INFO_RESPONSE_STR, dhi);

    map<string, string, std::less<>> pathInfoAttrs;
    pathInfoAttrs[PATH] = container;

    info->begin_tag(PATH_INFO_RESPONSE, &pathInfoAttrs);

    string validPath, remainder;
    bool isFile, isDir, canRead;
    long long size, time;

#if 0
    BESCatalogUtils *utils = BESCatalogUtils::Utils(defcatname);
#endif

    BESCatalogUtils *utils = BESCatalogList::TheCatalogList()->default_catalog()->get_catalog_utils();
    eval_resource_path(container, utils->get_root_dir(), utils->follow_sym_links(), validPath, isFile, isDir, size,
        time, canRead, remainder);

    // Now that we know what part of the path is actually something
    // we can access, find out if the BES sees it as a dataset
    bool isData = false;

    // If the valid path is an empty string then we KNOW it's not a dataset
    if (validPath.size() != 0) {

        // Get the catalog entry.
        BESCatalogEntry *entry = 0;
        // string coi = dhi.data[CATALOG];
        entry = defcat->show_catalog(validPath, /*coi,*/entry);
        if (!entry) {
            string err = (string) "Failed to find the validPath node " + validPath
                + " this should not be possible. Some thing BAD is happening.";
            throw BESInternalError(err, __FILE__, __LINE__);
        }

        // Retrieve the valid services list
        list<string> services = entry->get_service_list();

        // See if there's an OPENDAP_SERVICE available for the node.
        if (services.size()) {
            list<string>::const_iterator si = services.begin();
            list<string>::const_iterator se = services.end();
            for (; si != se; si++) {
                if ((*si) == OPENDAP_SERVICE) isData = true;
            }
        }
    }

    map<string, string, std::less<>> validPathAttrs;
    validPathAttrs[IS_DATA] = isData ? "true" : "false";
    validPathAttrs[IS_FILE] = isFile ? "true" : "false";
    validPathAttrs[IS_DIR] = isDir ? "true" : "false";
    validPathAttrs[IS_ACCESSIBLE] = canRead ? "true" : "false";

    // Convert size to string and add as attribute
    std::ostringstream os_size;
    os_size << size;
    validPathAttrs[SIZE] = os_size.str();

    // Convert lmt to string and add as attribute
    std::ostringstream os_time;
    os_time << time;
    validPathAttrs[LMT] = os_time.str();

    info->add_tag(VALID_PATH, validPath, &validPathAttrs);
    info->add_tag(REMAINDER, remainder);

    info->end_tag(PATH_INFO_RESPONSE);

    // end the response object
    info->end_response();

    BESDEBUG(SPI_DEBUG_KEY, prolog << "END" << endl );

    }

    /** @brief transmit the response object built by the execute command
     * using the specified transmitter object
     *
     * If a response object was built then transmit it as text
     *
     * @param transmitter object that knows how to transmit specific basic types
     * @param dhi structure that holds the request and response information
     * @see BESInfo
     * @see BESTransmitter
     * @see BESDataHandlerInterface
     */
void GatewayPathInfoResponseHandler::transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi)
{
    if (_response) {
        BESInfo *info = dynamic_cast<BESInfo *>(_response);
        if (!info) throw BESInternalError("cast error", __FILE__, __LINE__);
        info->transmit(transmitter, dhi);
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void GatewayPathInfoResponseHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "GatewayPathInfoResponseHandler::dump - (" << (void *) this << ")" << std::endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *
GatewayPathInfoResponseHandler::GatewayPathInfoResponseBuilder(const string &name)
{
    return new GatewayPathInfoResponseHandler(name);
}

/**
 *
 */
void GatewayPathInfoResponseHandler::eval_resource_path(const string &resource_path, const string &catalog_root,
    const bool follow_sym_links, string &validPath, bool &isFile, bool &isDir, long long &size,
    long long &lastModifiedTime, bool &canRead, string &remainder)
{

    BESDEBUG(SPI_DEBUG_KEY, prolog << "CatalogRoot: "<< catalog_root << endl);

    BESDEBUG(SPI_DEBUG_KEY, prolog << "resourceID: "<< resource_path << endl);

    // nothing valid yet...
    validPath = "";
    size = -1;
    lastModifiedTime = -1;

    // It's all remainder at this point...
    string rem = resource_path;
    remainder = rem;

    // Rather than have two basically identical code paths for the two cases (follow and !follow symlinks)
    // We evaluate the follow_sym_links switch and use a function pointer to get the correct "stat"
    // function for the eval operation.
    int (*ye_old_stat_function)(const char *pathname, struct stat *buf);
    if (follow_sym_links) {
        BESDEBUG(SPI_DEBUG_KEY, prolog << "Using 'stat' function (follow_sym_links = true)" << endl);
        ye_old_stat_function = &stat;
    }
    else {
        BESDEBUG(SPI_DEBUG_KEY, prolog << "Using 'lstat' function (follow_sym_links = false)" << endl);
        ye_old_stat_function = &lstat;
    }

    // if nothing is passed in path, then the path checks out since root is
    // assumed to be valid.
    if (resource_path == "") {
        BESDEBUG(SPI_DEBUG_KEY, prolog << "The resourceID is empty" << endl);
        return;
    }

    // make sure there are no ../ in the path, backing up in any way is
    // not allowed.
    string::size_type dotdot = resource_path.find("..");
    if (dotdot != string::npos) {
        BESDEBUG(SPI_DEBUG_KEY,
                 prolog << "ERROR: The resourceID '" << resource_path <<
                 "' contains the substring '..' This is Forbidden." << endl);
        string s = (string) "Invalid node name '" + resource_path + "' ACCESS IS FORBIDDEN";
        throw BESForbiddenError(s, __FILE__, __LINE__);
    }

    // What I want to do is to take each part of path and check to see if it
    // is a symbolic link and it is accessible. If everything is ok, add the
    // next part of the path.
    bool done = false;

    // Full file system path to check
    string fullpath = catalog_root;

    // localId that we are checking
    string checking;

    isFile = false;
    isDir = false;

    while (!done) {
        size_t slash = rem.find('/');
        if (slash == string::npos) {
            BESDEBUG(SPI_DEBUG_KEY, prolog << "Checking final path component: " << rem << endl);
            fullpath = BESUtil::assemblePath(fullpath, rem, true);
            checking = BESUtil::assemblePath(validPath, rem, true);
            rem = "";
            done = true;
        }
        else {
            fullpath = BESUtil::assemblePath(fullpath, rem.substr(0, slash), true);
            checking = BESUtil::assemblePath(validPath, rem.substr(0, slash), true);
            rem = rem.substr(slash + 1, rem.size() - slash);
        }

        BESDEBUG(SPI_DEBUG_KEY, prolog << "validPath: "<< validPath << endl);
        BESDEBUG(SPI_DEBUG_KEY, prolog << " checking: "<< checking << endl);
        BESDEBUG(SPI_DEBUG_KEY, prolog << " fullpath: "<< fullpath << endl);

        BESDEBUG(SPI_DEBUG_KEY,  prolog << "     rem: "<< rem << endl);

        BESDEBUG(SPI_DEBUG_KEY, prolog << "remainder: "<< remainder << endl);

        struct stat sb;
        int statret = ye_old_stat_function(fullpath.c_str(), &sb);

        if (statret != -1) {
            // No Error then keep chugging along.
            validPath = checking;
            remainder = rem;
        }
        else {
            int errsv = errno;
            // stat failed, so not accessible. Get the error string,
            // store in error, and throw exception
            char *s_err = strerror(errsv);
            string error = "Unable to access node " + checking + ": ";
            if (s_err) {
                error = error + s_err;
            }
            else {
                error = error + "unknown access error";
            }
            BESDEBUG(SPI_DEBUG_KEY, prolog << "    error: " << error << "   errno: " << errno << endl);

            BESDEBUG(SPI_DEBUG_KEY, prolog << "remainder: '" << remainder << "'" << endl);

            // ENOENT means that the node wasn't found. Otherwise, access
            // is denied for some reason
            if (errsv != ENOENT && errsv != ENOTDIR) {
                throw BESForbiddenError(error, __FILE__, __LINE__);
            }

            // Are there slashes in the remainder?
            size_t s_loc = remainder.find('/');
            if (s_loc == string::npos) {
                // if there are no more slashes, we check to see if this final path component contains "."
                string basename = remainder;
                bool moreDots = true;
                while (moreDots) {
                    // working back from end of string, drop each dot (".") suffix until file system match or string gone
                    size_t d_loc = basename.find_last_of(".");
                    if (d_loc != string::npos) {
                        basename = basename.substr(0, d_loc);
                        BESDEBUG(SPI_DEBUG_KEY, prolog << "basename: "<< basename << endl);

                        string candidate_remainder = remainder.substr(basename.size());
                        BESDEBUG(SPI_DEBUG_KEY, prolog << "candidate_remainder: "<< candidate_remainder << endl);

                        string candidate_path = BESUtil::assemblePath(validPath, basename, true);
                        BESDEBUG(SPI_DEBUG_KEY, prolog << "candidate_path: "<< candidate_path << endl);

                        string full_candidate_path = BESUtil::assemblePath(catalog_root, candidate_path, true);
                        BESDEBUG(SPI_DEBUG_KEY, prolog << "full_candidate_path: "<< full_candidate_path << endl);

                        struct stat sb1;
                        int statret1 = ye_old_stat_function(full_candidate_path.c_str(), &sb1);
                        if (statret1 != -1) {
                            validPath = candidate_path;
                            remainder = candidate_remainder;
                            moreDots = false;
                        }
                    }
                    else {
                        BESDEBUG(SPI_DEBUG_KEY, prolog << "No dots in remainder: "<< remainder << endl);
                        moreDots = false;
                    }
                }
            }
            else {
                BESDEBUG(SPI_DEBUG_KEY, prolog << "Remainder has slash pollution: "<< remainder << endl);
                done = true;
            }
        }
        fullpath = BESUtil::assemblePath(catalog_root, validPath, true);

        statret = ye_old_stat_function(fullpath.c_str(), &sb);
        if (S_ISREG(sb.st_mode)) {
            BESDEBUG(SPI_DEBUG_KEY, prolog << "'"<< fullpath << "' Is regular file." << endl);
            isFile = true;
            isDir = false;
        }
        else if (S_ISDIR(sb.st_mode)) {
            BESDEBUG(SPI_DEBUG_KEY, prolog << "'"<< fullpath << "' Is directory." << endl);
            isFile = false;
            isDir = true;
        }
        else if (S_ISLNK(sb.st_mode)) {
            BESDEBUG(SPI_DEBUG_KEY, prolog << "'"<< fullpath << "' Is symbolic Link." << endl);
            string error = "Service not configured to traverse symbolic links as embodied by the node '" + checking
                + "' ACCESS IS FORBIDDEN";
            throw BESForbiddenError(error, __FILE__, __LINE__);
        }
        // sb.st_uid;
        // sb.st_uid;

        // Can we read le file?
        std::ifstream ifile(fullpath.c_str());
        canRead = ifile.good();

        size = sb.st_size;
        // I'm pretty sure that assigning st_mtime to a long long (when it is a time_t) is not a
        // good plan - time_t is either a 32- or 64-bit signed integer.
        //
        // But, see ESCatalogUtils::bes_get_stat_info(BESCatalogEntry *entry, struct stat &buf)
        // for code that probably does a more universal version. of this (and other things relative
        // to stat, like symbolic link following).
        //
        // I added this #if ... #endif because Linux does not have st_mtimespec in struct stat.
        // jhrg 2.24.18
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
        // Compute LMT by converting the time to milliseconds since epoch - because OLFS is picky
        lastModifiedTime = (sb.st_mtimespec.tv_sec * 1000) + (sb.st_mtimespec.tv_nsec / 1000000);
#else
        lastModifiedTime = sb.st_mtime;
#endif
    }
    BESDEBUG(SPI_DEBUG_KEY, prolog << " fullpath: " << fullpath << endl);
    BESDEBUG(SPI_DEBUG_KEY, prolog << "validPath: " << validPath << endl);
    BESDEBUG(SPI_DEBUG_KEY, prolog << "remainder: " << remainder << endl);
    BESDEBUG(SPI_DEBUG_KEY, prolog << "      rem: " << rem << endl);
    BESDEBUG(SPI_DEBUG_KEY, prolog << "   isFile: " << (isFile?"true":"false") << endl);
    BESDEBUG(SPI_DEBUG_KEY, prolog << "    isDir: " << (isDir?"true":"false") << endl);
    BESDEBUG(SPI_DEBUG_KEY, prolog << "   access: " << (canRead?"true":"false") << endl);
    BESDEBUG(SPI_DEBUG_KEY, prolog << "     size: " << size << endl);
    BESDEBUG(SPI_DEBUG_KEY, prolog << "      LMT: " << lastModifiedTime << endl);

}
