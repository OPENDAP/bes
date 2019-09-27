//////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2009 OPeNDAP, Inc.
// Author: Michael Johnson  <m.johnson@opendap.org>
//
// For more information, please also see the main website: http://opendap.org/
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
// Please see the files COPYING and COPYRIGHT for more information on the GLPL.
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "DirectoryUtil.h"

#include <cstring>
#include <cerrno>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

// libdap
#include "GNURegex.h"

// bes
#include "BESDebug.h"
#include "BESForbiddenError.h"
#include "BESInternalError.h"
#include "TheBESKeys.h"
#include "BESNotFoundError.h"
#include "BESUtil.h"

using std::string;
using std::vector;
using std::endl;

namespace agg_util {
/**
 * Local file RAII wrapper for the DIR* so it's close regardless of
 * exception stack unwinding or what have you.
 */
struct DirWrapper {
public:

    DirWrapper(const string& fullDirPath) :
        _pDir(0), _fullPath(fullDirPath)
    {
        // if the user sees null after this, they can check the errno.
        _pDir = opendir(fullDirPath.c_str());
    }

    ~DirWrapper()
    {
        if (_pDir) {
            closedir(_pDir);
            _pDir = 0;
        }
    }

    bool fail() const
    {
        return !_pDir;
    }

    DIR*
    get() const
    {
        return _pDir;
    }

    // automatically closedir() if non-null on dtor.
    DIR* _pDir;
    std::string _fullPath;
};

/////////////////////// class FileInfo ////////////////////////////////
FileInfo::FileInfo(const std::string& path, const std::string& basename, bool isDir, time_t modTime) :
    _path(path), _basename(basename), _fullPath("") // start empty, cached later
        , _isDir(isDir), _modTime(modTime)
{
    DirectoryUtil::removeTrailingSlashes(_path);
    DirectoryUtil::removePrecedingSlashes(_basename);
}

FileInfo::~FileInfo()
{
}

const std::string&
FileInfo::path() const
{
    return _path;
}

const std::string&
FileInfo::basename() const
{
    return _basename;
}

bool FileInfo::isDir() const
{
    return _isDir;
}

time_t FileInfo::modTime() const
{
    return _modTime;
}

std::string FileInfo::getModTimeAsString() const
{
    // we'll just use UTC for the output...
    struct tm* pTM = gmtime(&_modTime);
    char buf[128];
    // this should be "Year-Month-Day Hour:Minute:Second"
    strftime(buf, 128, "%F %T", pTM);
    return string(buf);
}

const std::string&
FileInfo::getFullPath() const
{
    if (_fullPath.empty()) {
        _fullPath = _path + "/" + _basename;
    }
    return _fullPath;
}

std::string FileInfo::toString() const
{
    return "{FileInfo fullPath=" + getFullPath() + " isDir=" + ((isDir()) ? ("true") : ("false")) + " modTime=\""
        + getModTimeAsString() + "\""
            " }";
}

/////////////////////// class DirectoryUtil ////////////////////////////////

const string DirectoryUtil::_sDebugChannel = "agg_util";

DirectoryUtil::DirectoryUtil() :
    _rootDir("/"), _suffix("") // we start with no filter
        , _pRegExp(0), _filteringModTimes(false), _newestModTime(0L)
{
    // this can throw, but the class is completely constructed by this point.
    setRootDir("/");
}

DirectoryUtil::~DirectoryUtil()
{
    clearRegExp();
}

/** get the current root dir */
const std::string&
DirectoryUtil::getRootDir() const
{
    return _rootDir;
}

/** Makes sure the directory exists and is readable or throws an parse exception.
 Removes any trailing slashes unless the root is "/" (default).
 If noRelativePaths, then:
 Throws a parse exception if there is a "../" in the path somewhere.
 */
void DirectoryUtil::setRootDir(const std::string& origRootDir, bool allowRelativePaths/*=false*/,
    bool /*allowSymLinks=false*/)
{
    if (!allowRelativePaths && hasRelativePath(origRootDir)) {
        throw BESForbiddenError("can't use rootDir=" + origRootDir + " since it has a relative path (../)", __FILE__,
            __LINE__);
    }

    // Get the root without trailing slash, we'll add it.
    _rootDir = origRootDir;
    removeTrailingSlashes(_rootDir);
    // If empty here, that means the actual filesystem root.

    // Use the BESUtil to test the path
    // Since it assumes root is valid and strips preceding "/",
    // we use "/" as the root path and the root path as the path
    // to validate the root.  This will throw if invalid.
    BESUtil::check_path(_rootDir, "/", false); // not going to allow symlinks by default.

    // We should be good if we get here.
}

void DirectoryUtil::setFilterSuffix(const std::string& suffix)
{
    _suffix = suffix;
}

void DirectoryUtil::setFilterRegExp(const std::string& regexp)
{
    clearRegExp();  // avoid leaks
    if (!regexp.empty()) {
        _pRegExp = new libdap::Regex(regexp.c_str());
    }
}

void DirectoryUtil::clearRegExp()
{
    delete _pRegExp;
    _pRegExp = 0;
}

void DirectoryUtil::setFilterModTimeOlderThan(time_t newestModTime)
{
    _newestModTime = newestModTime;
    _filteringModTimes = true;
}

void DirectoryUtil::getListingForPath(const std::string& path, std::vector<FileInfo>* pRegularFiles,
    std::vector<FileInfo>* pDirectories)
{
    string pathToUse(path);
    removePrecedingSlashes(pathToUse);
    pathToUse = getRootDir() + "/" + pathToUse;
    BESDEBUG(_sDebugChannel, "Attempting to get dir listing for path=\"" << pathToUse << "\"" << endl);

    // RAII, will closedir no matter how we leave function, including a throw
    DirWrapper pDir(pathToUse);
    if (pDir.fail()) {
        throwErrorForOpendirFail(pathToUse);
    }

    // Go through each entry and see if it's a directory or regular file and
    // add it to the list.
    struct dirent* pDirEnt = 0;
    while ((pDirEnt = readdir(pDir.get())) != 0) {
        string entryName = pDirEnt->d_name;
        // Exclude ".", ".." and any dotfile dirs like ".svn".
        if (!entryName.empty() && entryName[0] == '.') {
            continue;
        }

        // Figure out if it's a regular file or directory
        string pathToEntry = pathToUse + "/" + entryName;
        struct stat statBuf;
        int statResult = stat(pathToEntry.c_str(), &statBuf);
        if (statResult != 0) {
            // If we can't stat the file for some reason, then ignore it
            continue;
        }

        // Use the passed in path for the entry since we
        // want to make the locations be relative to the root
        // for loading later.
        if (pDirectories && S_ISDIR(statBuf.st_mode)) {
            pDirectories->push_back(FileInfo(path, entryName, true, statBuf.st_mtime));
        }
        else if (pRegularFiles && S_ISREG(statBuf.st_mode)) {
            FileInfo theFile(path, entryName, false, statBuf.st_mtime);
            // match against the relative passed in path, not root full path
            if (matchesAllFilters(theFile.getFullPath(), statBuf.st_mtime)) {
                pRegularFiles->push_back(theFile);
            }
        }
    }
}

void DirectoryUtil::getListingForPathRecursive(const std::string& path, std::vector<FileInfo>* pRegularFiles,
    std::vector<FileInfo>* pDirectories)
{
    // Remove trailing slash to make it canonical
    string canonicalPath = path;
    removeTrailingSlashes(canonicalPath);

    // We use our own local vector of directories in order to recurse,
    // then add them to the end of pDirectories if it exists.

    // First, get the current path's listing
    vector<FileInfo> dirs;
    dirs.reserve(16); // might as well start with a "few" to avoid grows.

    // Keep adding them to the user specified regular file list if desired,
    // but keep track of dirs ourself.
    getListingForPath(canonicalPath, pRegularFiles, &dirs);

    // If the caller wanted directories, append them all to the return
    if (pDirectories) {
        pDirectories->insert(pDirectories->end(), dirs.begin(), dirs.end());
    }

    // Finally, recurse on each directory in dirs
    for (vector<FileInfo>::const_iterator it = dirs.begin(); it != dirs.end(); ++it) {
        string subPath = canonicalPath + "/" + it->basename();
        BESDEBUG(_sDebugChannel, "DirectoryUtil: recursing down to directory subtree=\"" << subPath << "\"..." << endl);
        // Pass down the caller's accumulated vector's to be filled in.
        getListingForPathRecursive(subPath, pRegularFiles, pDirectories);
    }

}

void DirectoryUtil::getListingOfRegularFilesRecursive(const std::string& path, std::vector<FileInfo>& rRegularFiles)
{
    // call the other one, not accumulated the directories, only recursing into them.
    getListingForPathRecursive(path, &rRegularFiles, 0);
}

void DirectoryUtil::throwErrorForOpendirFail(const string& fullPath)
{
    switch (errno) {
    case EACCES: {
        string msg = "Permission denied for some directory in path=\"" + fullPath + "\"";
        throw BESForbiddenError(msg, __FILE__, __LINE__);
    }
        break;

    case ELOOP: {
        string msg = "A symlink loop was detected in path=\"" + fullPath + "\"";
        throw BESNotFoundError(msg, __FILE__, __LINE__);  // closest I can figure...
    }
        break;

    case ENAMETOOLONG: {
        string msg = "A name in the path was too long.  path=\"" + fullPath + "\"";
        throw BESNotFoundError(msg, __FILE__, __LINE__);
    }
        break;

    case ENOENT: {
        string msg = "Some part of the path was not found.  path=\"" + fullPath + "\"";
        throw BESNotFoundError(msg, __FILE__, __LINE__);
    }
        break;

    case ENOTDIR: {
        string msg = "Some part of the path was not a directory. path=\"" + fullPath + "\"";
        throw BESNotFoundError(msg, __FILE__, __LINE__);
    }
        break;

    case ENFILE: {
        string msg = "Internal Error: Too many files are currently open!";
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
        break;

    default: {
        string msg = "An unknown errno was found after opendir() was called on path=\"" + fullPath + "\"";
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    }
}

bool DirectoryUtil::matchesAllFilters(const std::string& path, time_t modTime) const
{
    bool matches = true;
    // Do the suffix first since it's fast
    if (!_suffix.empty() && !matchesSuffix(path, _suffix)) {
        matches = false;
    }

    // Suffix matches and we have a regexp, check that
    if (matches && _pRegExp) {
        // match the full string, -1 on fail, num chars matching otherwise
        int numCharsMatching = _pRegExp->match(path.c_str(), path.size(), 0);
        matches = (numCharsMatching > 0); // TODO do we want to match the size()?
    }

    if (matches && _filteringModTimes) {
        matches = (modTime < _newestModTime);
    }

    return matches;
}

bool DirectoryUtil::hasRelativePath(const std::string& path)
{
    return (path.find("..") != string::npos);
}

void DirectoryUtil::removeTrailingSlashes(string& path)
{
    if (!path.empty()) {
        string::size_type pos = path.find_last_not_of("/");
        if (pos != string::npos) {
            path = path.substr(0, pos + 1);
        }
    }
}

void DirectoryUtil::removePrecedingSlashes(std::string& path)
{
    if (!path.empty()) {
        string::size_type pos = path.find_first_not_of("/");
        path = path.substr(pos, string::npos);
    }
}

void DirectoryUtil::printFileInfoList(const vector<FileInfo>& listing)
{
    std::ostringstream oss;
    printFileInfoList(oss, listing);
    BESDEBUG(_sDebugChannel, oss.str() << endl);
}

void DirectoryUtil::printFileInfoList(std::ostream& os, const vector<FileInfo>& listing)
{
    for (vector<FileInfo>::const_iterator it = listing.begin(); it != listing.end(); ++it) {
        os << it->toString() << endl;
    }
}

std::string DirectoryUtil::getBESRootDir()
{
    bool found;
    string rootDir;
    TheBESKeys::TheKeys()->get_value("BES.Catalog.catalog.RootDirectory", rootDir, found);
    if (!found) {
        TheBESKeys::TheKeys()->get_value("BES.Data.RootDirectory", rootDir, found);
    }
    if (!found) {
        rootDir = "/";
    }
    return rootDir;
}

bool DirectoryUtil::matchesSuffix(const std::string& filename, const std::string& suffix)
{
    // see if the last suffix.size() characters match.
    bool matches = (filename.find(suffix, filename.size() - suffix.size()) != string::npos);
    return matches;
}
}
