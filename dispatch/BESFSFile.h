// BESFSFile.h

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

#ifndef I_BESFSFile_h
#define I_BESFSFile_h

#include <string>

#include "BESFSFile.h"

class BESFSFile {
private:
    std::string _dirName;
    std::string _fileName;
    std::string _baseName;
    std::string _extension;

    void breakApart(const std::string &fullPath);
    void breakExtension();

public:
    BESFSFile(const std::string &fullPath);
    BESFSFile(const std::string &dirName, const std::string &fileName);
    BESFSFile(const BESFSFile &copyFrom);
    virtual ~BESFSFile();

    virtual std::string getDirName();
    virtual std::string getFileName();
    virtual std::string getBaseName();
    virtual std::string getExtension();
    virtual std::string getFullPath();

    virtual bool exists(std::string &reason);
    virtual bool isReadable(std::string &reason);
    virtual bool isWritable(std::string &reason);
    virtual bool isExecutable(std::string &reason);
    virtual bool hasDotDot();
};

#endif // I_BESDir_h
