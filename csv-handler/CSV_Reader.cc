// CSV_Reader.cc

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#include"CSV_Reader.h"

CSV_Reader::CSV_Reader() {
  stream_in = new fstream();
}

CSV_Reader::~CSV_Reader() {
  if(stream_in->is_open())
    stream_in->close();
  delete stream_in;
}

const bool CSV_Reader::open(const string& filepath) {
  stream_in->open(filepath.c_str(),fstream::in);
  if(stream_in->fail() or !(stream_in->is_open()))
    return false;
  else
    return true;
}

const bool CSV_Reader::close() {
  stream_in->close();
  if(stream_in->bad() || stream_in->is_open())
    return false;
  else
    return true;
}

const bool CSV_Reader::eof() {
  return stream_in->eof();
}

void CSV_Reader::reset() {
  stream_in->seekg(ios::beg);
}

vector<string> CSV_Reader::get() {
  vector<string> foo;
  string bar;

  getline(*stream_in, bar);
  foo = split(bar,",");
  return foo;
}

vector<string> split(const string& str, const string& delimiters) {
  vector<string> tokens;
  string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  string::size_type pos = str.find_first_not_of(delimiters, lastPos);

  while(string::npos != pos || string::npos != lastPos) {
    if(lastPos != pos)
      tokens.push_back(str.substr(lastPos, pos - lastPos));
    lastPos = str.find_first_not_of(delimiters, pos);
    pos = str.find_first_of(delimiters, lastPos);
  }

  return tokens;
}

