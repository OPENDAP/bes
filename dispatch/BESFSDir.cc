// BESFSDir.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef WIN32
#include <config.h> //  for S_ISDIR macro
#endif
#include <cstdio>

#include "BESFSDir.h"
#include "BESInternalError.h"
#include "BESRegex.h"

using std::string;

BESFSDir::BESFSDir(const string &dirName) : _dirName(dirName), _fileExpr(""), _dirLoaded(false) {}

BESFSDir::BESFSDir(const string &dirName, const string &fileExpr)
    : _dirName(dirName), _fileExpr(fileExpr), _dirLoaded(false) {}

BESFSDir::BESFSDir(const BESFSDir &copyFrom)
    : _dirName(copyFrom._dirName), _fileExpr(copyFrom._fileExpr), _dirLoaded(false) {}

BESFSDir::~BESFSDir() = default;

BESFSDir::dirIterator BESFSDir::beginOfDirList() {
    if (_dirLoaded == false) {
        loadDir();
        _dirLoaded = true;
    }
    return _dirList.begin();
}

BESFSDir::dirIterator BESFSDir::endOfDirList() {
    if (_dirLoaded == false) {
        loadDir();
        _dirLoaded = true;
    }
    return _dirList.end();
}

BESFSDir::fileIterator BESFSDir::beginOfFileList() {
    if (_dirLoaded == false) {
        loadDir();
        _dirLoaded = true;
    }
    return _fileList.begin();
}

BESFSDir::fileIterator BESFSDir::endOfFileList() {
    if (_dirLoaded == false) {
        loadDir();
        _dirLoaded = true;
    }
    return _fileList.end();
}

void BESFSDir::loadDir() {
    DIR *dip;
    struct dirent *dit;

    try {
        // open a directory stream
        // make sure the directory is valid and readable
        if ((dip = opendir(_dirName.c_str())) == nullptr) {
            string err_str = "ERROR: failed to open directory '" + _dirName + "'";
            throw BESError(err_str, BES_NOT_FOUND_ERROR, __FILE__, __LINE__);
        } else {
            // read in the files in this directory
            // add each filename to the list of filenames
            while ((dit = readdir(dip)) != nullptr) {
                struct stat buf;
                string dirEntry = dit->d_name;
                if (dirEntry != "." && dirEntry != "..") {
                    string fullPath = _dirName + "/" + dirEntry;

                    // This test used to throw a BES_NOT_FOUND_ERROR which I don't think is
                    // correct. If opendir() is used to open _dirName and iterate over the
                    // entries returned by repeated calls to readdir() then how can the
                    // paths be bad? One way, it turns out, is if there is a second process
                    // adding and removing files in _dirName. But I think it's better to
                    // ignore that case then return an error - is it really a BES error if
                    // another process is rude? This showed up when running parallel tests
                    // because the tests keysT and pvolT were stepping on each other. However,
                    // I don't think this is limited to 'make check -j9' since it could be
                    // exploited to crash the bes. jhrg 3/30/17
                    if (-1 == stat(fullPath.c_str(), &buf))
                        continue;
#if 0
                        // Replaced by 'continue'. jhrg 3/30/17
                        throw BESError(string("Did not find the path: '") + fullPath + "'", BES_NOT_FOUND_ERROR,
                            __FILE__, __LINE__);
#endif
                    // look at the mode and determine if this is a filename
                    // or a directory name
                    if (S_ISDIR(buf.st_mode)) {
                        _dirList.emplace_back(fullPath);
                    } else {
                        if (_fileExpr != "") {
                            BESRegex reg_expr(_fileExpr.c_str());
                            int match_ret = reg_expr.match(dirEntry.c_str(), dirEntry.size());
                            if (match_ret == static_cast<int>(dirEntry.size())) {
                                _fileList.emplace_back(_dirName, dirEntry);
                            }
                        } else {
                            _fileList.emplace_back(_dirName, dirEntry);
                        }
                    }
                }
            }
        }

        // close the directory
        closedir(dip);
    } catch (...) {
        // close the directory
        closedir(dip);
        throw;
    }
}
