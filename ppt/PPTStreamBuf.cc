// PPTStreamBuf.cc

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

#include <sys/types.h>

#include <cstdio>
#include <unistd.h> // for sync
#include <sstream>
#include <iomanip>
#include <iostream>

using std::ostringstream;
using std::hex;
using std::setw;
using std::setfill;

#include "PPTStreamBuf.h"

const char* eod_marker = "0000000d";
const size_t eod_marker_len = 8;

PPTStreamBuf::PPTStreamBuf(int fd, unsigned bufsize) :
    d_bufsize(bufsize), d_buffer(0), count(0)
{
    open(fd, bufsize);
}

PPTStreamBuf::~PPTStreamBuf()
{
    if (d_buffer) {
        sync();
        delete[] d_buffer;
    }
}

void PPTStreamBuf::open(int fd, unsigned bufsize)
{
    d_fd = fd;
    d_bufsize = bufsize == 0 ? 1 : bufsize;

    d_buffer = new char[d_bufsize];
    setp(d_buffer, d_buffer + d_bufsize);
}

// We're stuck with this return type because this is inherited from stdc++ streambuf. jhrg
int PPTStreamBuf::sync()
{
    if (pptr() > pbase()) {
        ostringstream strm;
        strm << hex << setw(7) << setfill('0') << (unsigned int) (pptr() - pbase()) << "d";
        write(d_fd, strm.str().c_str(), strm.str().length());

        count += write(d_fd, d_buffer, pptr() - pbase());
        setp(d_buffer, d_buffer + d_bufsize);
    }

    return 0;
}

int PPTStreamBuf::overflow(int c)
{
    sync();
    if (c != EOF) {
        *pptr() = static_cast<char>(c);
        pbump(1);
    }
    return c;
}

void PPTStreamBuf::finish()
{
    sync();

    write(d_fd, eod_marker, eod_marker_len); // tmp_str.c_str(), tmp_str.length() ) ;

    count = 0;
}

