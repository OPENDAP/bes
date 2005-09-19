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
// along with this library; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
// $RCSfile: dhdferr.h,v $ - HDF server error class declarations
//
// $Log: dhdferr.h,v $
// Revision 1.4.18.1  2003/05/21 16:26:55  edavis
// Updated/corrected copyright statements.
//
// Revision 1.4  1999/05/06 03:23:35  jimg
// Merged changes from no-gnu branch
//
// Revision 1.3.20.1  1999/05/06 00:27:23  jimg
// Jakes String --> string changes
//
// Revision 1.3  1997/03/10 22:45:48  jimg
// Update for 2.12
//
// Revision 1.1  1996/09/27 18:21:32  todd
// Initial revision
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _DHDFERR_H
#define _DHDFERR_H

#include <iostream>
#include <string>
#include <hcerr.h>

class dhdferr;

#ifdef NO_EXCEPTIONS
#define THROW(x) fakethrow(x(__FILE__,__LINE__))
void fakethrow(const hcerr&);
void fakethrow(const dhdferr&);
#else
#define THROW(x) throw x(__FILE__,__LINE__)
#endif

// DODS/HDF exceptions class
class dhdferr {
public:
    dhdferr(const string& msg, const string& file, int line) : 
         _errmsg(msg), _file(file), _line(line) {}
    virtual ~dhdferr(void) {}
    friend ostream& operator<<(ostream& out, const dhdferr& x);
    string errmsg(void) const { return _errmsg; }
    string file(void) const { return _file; }
    int line(void) const { return _line; }
protected:
    virtual void _print(ostream& out) const;
    string _errmsg;
    string _file;
    int _line;
};

// Define valid DODS/HDF exceptions
class dhdferr_hcerr: public dhdferr {
public:
    dhdferr_hcerr(const string& msg, const string& file, int line): 
         dhdferr(msg, file, line) {}
protected:
    virtual void _print(ostream& out) const;
}; // An hdfclass error occured

class dhdferr_addattr: public dhdferr {
public:
    dhdferr_addattr(const string& file, int line): 
         dhdferr(string("Error occurred while trying to add attribute to DAS"), 
		 file, line) {}
}; // AttrTable::append_attr() failed

class dhdferr_ddssem: public dhdferr {
public:
    dhdferr_ddssem(const string& file, int line): 
         dhdferr(string("Problem with DDS semantics"), file, line) {}
}; // DDS::check_semantics() returned false

class dhdferr_dassem: public dhdferr {
public:
    dhdferr_dassem(const string& file, int line): 
         dhdferr(string("Problem with DAS semantics"), file, line) {}
}; // DAS::check_semantics() returned false

class dhdferr_ddsout: public dhdferr {
public:
    dhdferr_ddsout(const string& file, int line): 
         dhdferr(string("Could not write to DDS file"), file, line) {}
}; // error writing to DDS file

class dhdferr_dasout: public dhdferr {
public:
    dhdferr_dasout(const string& file, int line): 
         dhdferr(string("Could not write to DAS file"), file, line) {}
}; // error writing to DAS file

class dhdferr_arrcons: public dhdferr {
public:
    dhdferr_arrcons(const string& file, int line): 
         dhdferr(string("Error occurred while reading Array constraint"), 
		 file, line) {}
}; // constraints read for Array slab did not make sense

class dhdferr_datatype: public dhdferr {
public:
    dhdferr_datatype(const string& file, int line): 
         dhdferr(string("Data type is not supported by DODS"), 
		 file, line) {}
}; // data type is not supported by DODS

class dhdferr_consist: public dhdferr {
public:
    dhdferr_consist(const string& file, int line): 
         dhdferr(string("Internal consistency problem"), 
		 file, line) {}
}; // something that is not supposed to happen did

class dhdferr_hcread: public dhdferr {
public:
    dhdferr_hcread(const string& file, int line): 
         dhdferr(string("Problem reading HDF data"), 
		 file, line) {}
}; // something went wrong when reading using the HDFCLASS library

class dhdferr_conv: public dhdferr {
public:
    dhdferr_conv(const string& file, int line): 
         dhdferr(string("Problem converting HDF data to DODS"), 
		 file, line) {}
}; // something went wrong in a conversion to DAP

class dhdferr_fexist: public dhdferr {
public:
    dhdferr_fexist(const string& file, int line): 
         dhdferr(string("HDF file does not exist or is unreadable"), 
		 file, line) {}
}; // could not stat the input HDF file

#endif // _DHDFERR_H
