// PPTStreamBuf.h

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

#ifndef I_PPTStreamBuf_h
#define I_PPTStreamBuf_h 1

#include <streambuf>

class PPTStreamBuf: public std::streambuf {
private:
    unsigned d_bufsize;
    int d_fd;
    char * d_buffer;
    unsigned int count;

    PPTStreamBuf() :
        d_bufsize(0), d_fd(-1), d_buffer(0), count(0)
    {
    }
public:
    PPTStreamBuf(int fd, unsigned bufsize = 1);
    virtual ~PPTStreamBuf();

    unsigned int how_many()
    {
        return count;
    }

    void open(int fd, unsigned bufsize = 1);

    int sync();

    int overflow(int c);

    void finish();
};

#endif // I_PPTStreamBuf_h 1

