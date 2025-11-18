// BESFSFile.cc

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

#include "config.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <cerrno>
#include <cstring>

#include "BESFSFile.h"

using std::string;

BESFSFile::BESFSFile(const string &fullPath) : _dirName(""), _fileName(""), _baseName(""), _extension("") {
    breakApart(fullPath);
}

BESFSFile::BESFSFile(const string &dirName, const string &fileName)
    : _dirName(dirName), _fileName(fileName), _baseName(""), _extension("") {
    breakExtension();
}

BESFSFile::BESFSFile(const BESFSFile &copyFrom)
    
      = default;

BESFSFile::~BESFSFile() = default;

string BESFSFile::getDirName() { return _dirName; }

string BESFSFile::getFileName() { return _fileName; }

string BESFSFile::getBaseName() { return _baseName; }

string BESFSFile::getExtension() { return _extension; }

string BESFSFile::getFullPath() { return _dirName + "/" + _fileName; }

void BESFSFile::breakApart(const string &fullPath) {
    string::size_type pos = fullPath.rfind("/");
    if (pos != string::npos) {
        _dirName = fullPath.substr(0, pos);
        _fileName = fullPath.substr(pos + 1, fullPath.size() - pos);
    } else {
        _dirName = "./";
        _fileName = fullPath;
    }

    breakExtension();
}

void BESFSFile::breakExtension() {
    string::size_type pos = _fileName.rfind(".");
    if (pos != string::npos) {
        _baseName = _fileName.substr(0, pos);
        _extension = _fileName.substr(pos + 1, _fileName.size() - pos);
    } else {
        _baseName = _fileName;
    }
}

bool BESFSFile::exists(string &reason) {
    bool ret = false;
    if (!access(getFullPath().c_str(), F_OK)) {
        ret = true;
    } else {
        char *err = strerror(errno);
        if (err) {
            reason += err;
        } else {
            reason += "Unknown error";
        }
    }
    return ret;
}

bool BESFSFile::isReadable(string &reason) {
    bool ret = false;
    if (!access(getFullPath().c_str(), R_OK)) {
        ret = true;
    } else {
        char *err = strerror(errno);
        if (err) {
            reason += err;
        } else {
            reason += "Unknown error";
        }
    }
    return ret;
}

bool BESFSFile::isWritable(string &reason) {
    bool ret = false;
    if (!access(getFullPath().c_str(), W_OK)) {
        ret = true;
    } else {
        char *err = strerror(errno);
        if (err) {
            reason += err;
        } else {
            reason += "Unknown error";
        }
    }
    return ret;
}

bool BESFSFile::isExecutable(string &reason) {
    bool ret = false;
    if (!access(getFullPath().c_str(), X_OK)) {
        ret = true;
    } else {
        char *err = strerror(errno);
        if (err) {
            reason += err;
        } else {
            reason += "Unknown error";
        }
    }
    return ret;
}

bool BESFSFile::hasDotDot() {
    bool ret = false;
    string fp = getFullPath();
    if (fp.find("..") != string::npos) {
        ret = true;
    }
    return ret;
}
