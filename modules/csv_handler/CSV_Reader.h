// CSV_Reader.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Stephan Zednik <zednik@ucar.edu> and Patrick West <pwest@ucar.edu>
// and Jose Garcia <jgarcia@ucar.edu>
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
//	zednik      Stephan Zednik <zednik@ucar.edu>
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef I_CSV_Reader_h
#define I_CSV_Reader_h 1

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>

#include <BESObj.h>

class CSV_Reader : public BESObj {
private:
    unsigned long long _row_number;
    std::string _filepath;
    std::fstream *_stream_in;
public:
    CSV_Reader();

    virtual ~CSV_Reader();

    bool open(const std::string &filepath);

    bool close() const;

    bool eof() const;

    void reset();

    void get(std::vector<std::string> &row);

    unsigned long long get_row_number() const { return _row_number; }
    std::string get_file_name() const { return _filepath; }

    void dump(std::ostream &strm) const override;
};

#endif // I_CSV_Reader_h
