/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1996, California Institute of Technology.  
// ALL RIGHTS RESERVED.   U.S. Government Sponsorship acknowledged. 
//
// Please read the full copyright notice in the file COPYRIGH 
// in this directory.
//
// Author: Todd Karakashian, NASA/Jet Propulsion Laboratory
//         Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: dhdferr.h,v $ - HDF server error class declarations
//
// $Log: dhdferr.h,v $
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
