/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999, University of Rhode Island
// ALL RIGHTS RESERVED. 
//
// Please read the full copyright notice in the file COPYRIGHT in
// DODS_Root/etc

// Author: James Gallagher
//
// $Log: HDFInt16.cc,v $
// Revision 1.2  1999/05/06 00:27:22  jimg
// Jakes String --> string changes
//
// Revision 1.1  1999/03/27 00:20:16  jimg
// Added
//

#include "HDFInt16.h"

HDFInt16::HDFInt16(const string &n = (char *)0) : Int16(n) {}
HDFInt16::~HDFInt16() {}
BaseType *HDFInt16::ptr_duplicate() { return new HDFInt16(*this); }
bool HDFInt16::read(const string &, int &err) { 
    set_read_p(true); err = 1; return false; 
}

Int16 *NewInt16(const string &n) { return new HDFInt16(n); }
