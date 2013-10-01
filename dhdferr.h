// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
// 
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

/////////////////////////////////////////////////////////////////////////////
// Copyright 1996, by the California Institute of Technology.
// ALL RIGHTS RESERVED. United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the
// Office of Technology Transfer at the California Institute of
// Technology. This software may be subject to U.S. export control
// laws and regulations. By accepting this software, the user
// agrees to comply with all applicable U.S. export laws and
// regulations. User has the responsibility to obtain export
// licenses, or other export authority as may be required before
// exporting such information to foreign countries or providing
// access to foreign persons.

// Author: Todd Karakashian, NASA/Jet Propulsion Laboratory
//         Todd.K.Karakashian@jpl.nasa.gov
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _DHDFERR_H
#define _DHDFERR_H

#include <iostream>
#include <string>
#include <hcerr.h>

#include "Error.h"

using namespace libdap;

#define THROW(x) throw x(__FILE__,__LINE__)

// DODS/HDF exceptions class
class dhdferr:public Error {
  public:
    dhdferr(const string & msg, const string & file, int line);
     virtual ~ dhdferr(void) {
}};

// Define valid DODS/HDF exceptions
class dhdferr_hcerr:public dhdferr {
  public:
    dhdferr_hcerr(const string & msg, const string & file, int line);
};                              // An hdfclass error occured

class dhdferr_addattr:public dhdferr {
  public:
    dhdferr_addattr(const string & file,
                    int
                    line):dhdferr(string
                                  ("Error occurred while trying to add attribute to DAS"),
                                  file, line) {
}};                             // AttrTable::append_attr() failed

class dhdferr_ddssem:public dhdferr {
  public:
    dhdferr_ddssem(const string & file,
                   int line):dhdferr(string("Problem with DDS semantics"),
                                     file, line) {
}};                             // DDS::check_semantics() returned false

class dhdferr_dassem:public dhdferr {
  public:
    dhdferr_dassem(const string & file,
                   int line):dhdferr(string("Problem with DAS semantics"),
                                     file, line) {
}};                             // DAS::check_semantics() returned false

class dhdferr_ddsout:public dhdferr {
  public:
    dhdferr_ddsout(const string & file,
                   int line):dhdferr(string("Could not write to DDS file"),
                                     file, line) {
}};                             // error writing to DDS file

class dhdferr_dasout:public dhdferr {
  public:
    dhdferr_dasout(const string & file,
                   int line):dhdferr(string("Could not write to DAS file"),
                                     file, line) {
}};                             // error writing to DAS file

class dhdferr_arrcons:public dhdferr {
  public:
    dhdferr_arrcons(const string & file,
                    int
                    line):dhdferr(string
                                  ("Error occurred while reading Array constraint"),
                                  file, line) {
}};                             // constraints read for Array slab did not make sense

class dhdferr_datatype:public dhdferr {
  public:
    dhdferr_datatype(const string & file,
                     int
                     line):dhdferr(string
                                   ("Data type is not supported by DODS"),
                                   file, line) {
}};                             // data type is not supported by DODS

class dhdferr_consist:public dhdferr {
  public:
    dhdferr_consist(const string & file,
                    int
                    line):dhdferr(string("Internal consistency problem"),
                                  file, line) {
}};                             // something that is not supposed to happen did

class dhdferr_hcread:public dhdferr {
  public:
    dhdferr_hcread(const string & file,
                   int line):dhdferr(string("Problem reading HDF data"),
                                     file, line) {
}};                             // something went wrong when reading using the HDFCLASS library

class dhdferr_conv:public dhdferr {
  public:
    dhdferr_conv(const string & file,
                 int
                 line):dhdferr(string
                               ("Problem converting HDF data to DODS"),
                               file, line) {
}};                             // something went wrong in a conversion to DAP

class dhdferr_fexist:public dhdferr {
  public:
    dhdferr_fexist(const string & file,
                   int
                   line):dhdferr(string
                                 ("HDF file does not exist or is unreadable"),
                                 file, line) {
}};                             // could not stat the input HDF file

#endif                          // _DHDFERR_H
