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
// $RCSfile: hdfutil.cc,v $ - Miscellaneous routines for DODS HDF server
//
// $Log: hdfutil.cc,v $
// Revision 1.2  1997/02/10 02:01:58  jimg
// Update from Todd.
//
// Revision 1.3  1996/11/20  22:28:43  todd
// Modified to support UInt32 type.
//
// Revision 1.2  1996/10/07 21:15:17  todd
// Changes escape character to % from _.
//
// Revision 1.1  1996/09/24 22:38:16  todd
// Initial revision
//
//
/////////////////////////////////////////////////////////////////////////////

#include <strstream.h>
#include <iomanip.h>
#include <String.h>
#include <mfhdf.h>
#include <hdfclass.h>
#include "HDFStructure.h"
#include "HDFArray.h"
#include "hdfutil.h"
#include "dhdferr.h"

String hexstring(int val) {
    static char buf[hdfclass::MAXSTR];

    ostrstream(buf,hdfclass::MAXSTR) << hex << setw(2) << setfill('0') << 
	val << ends;

    return (String)buf;
}

char unhexstring(String s) {
    int val;
    static char buf[hdfclass::MAXSTR];

    strcpy(buf,(const char *)s);
    istrstream(buf,hdfclass::MAXSTR) >> hex >> val;

    return (char)val;
}

String octstring(int val) {
    static char buf[hdfclass::MAXSTR];

    ostrstream(buf,hdfclass::MAXSTR) << oct << setw(3) << setfill('0') << 
	val << ends;

    return (String)buf;
}

char unoctstring(String s) {
    int val;
    static char buf[hdfclass::MAXSTR];

    strcpy(buf,(const char *)s);
    istrstream(buf,hdfclass::MAXSTR) >> oct >> val;

    return (char)val;
}


// replace characters that are not allowed in DODS identifiers
String id2dods(String s) {
    static Regex badregx = "[^0-9a-zA-Z_%]";
    const char ESC = '%';

    int index;
    while ( (index = s.index(badregx)) >= 0)
	s.at(index,1) = ESC + hexstring(toascii(s[index]));
    if (isdigit(s[0]))
	s.before(0) = '_';
    return s;
}

String dods2id(String s) {
    static Regex escregx = "%[0-7][0-9a-fA-F]";

    int index;
    while ( (index = s.index(escregx)) >= 0)
	s.at(index,3) = unhexstring(s.at(index+1,2));

    return s;
}

// Escape non-printable characters and quotes from an HDF attribute
String escattr(String s) {
    static Regex nonprintable = "[^ !-~]";
    const char ESC = '\\';
    const char QUOTE = '\"';
    const String ESCQUOTE = (String)ESC + (String)QUOTE;

    // escape non-printing characters with octal escape
    int index = 0;
    while ( (index = s.index(nonprintable)) >= 0)
	s.at(index,1) = ESC + octstring(toascii(s[index]));

    // escape " with backslash
    index = 0;
    while ( (index = s.index(QUOTE,index)) >= 0) {
	s.at(index,1) = ESCQUOTE;
	index += ESCQUOTE.length();
    }
	   
    return s;
}

// Un-escape special characters, quotes and backslashes from an HDF attribute.
// Note: A regex to match one \ must be defined as
//          Regex foo = "\\\\";
//       because both C++ strings and libg++ regex's also employ \ as
//       an escape character!
String unescattr(String s) {
    static Regex escregx = "\\\\[01][0-7][0-7]";  // matches 4 characters
    static Regex escquoteregex = "[^\\\\]\\\\\"";  // matches 3 characters
    static Regex escescregex = "\\\\\\\\";      // matches 2 characters
    const char ESC = '\\';
    const char QUOTE = '\"';
    const String ESCQUOTE = (String)ESC + (String)QUOTE;

    // unescape any octal-escaped ASCII characters
    int index = 0;
    while ( (index = s.index(escregx,index)) >= 0) {
	s.at(index,4) = unoctstring(s.at(index+1,3));
	index++;
    }

    // unescape any escaped quotes
    index = 0;
    while ( (index = s.index(escquoteregex,index)) >= 0) {
	s.at(index+1,2) = QUOTE;
	index++;
    }

    // unescape any escaped backslashes
    index = 0;
    while ( (index = s.index(escescregex,index)) >= 0) {
	s.at(index,2) = ESC;
	index++;
    }
    
    return s;
}

void *ExportDataForDODS(const hdf_genvec& v) {
    void *rv;			// reminder: rv is an array; must be deleted
				// with delete[] not delete

    switch (v.number_type()) {
    case DFNT_INT8:
    case DFNT_INT16:
    case DFNT_INT32:
	rv = v.export_int32();
	break;
    case DFNT_UINT8:
    case DFNT_UINT16:
    case DFNT_UINT32:
	rv = v.export_uint32();
	break;
    case DFNT_FLOAT32: 
    case DFNT_FLOAT64: 
	rv = v.export_float64();
	break;
    case DFNT_UCHAR8:
	rv = v.export_uchar8();
	break;
    case DFNT_CHAR8:
//	rv = v.export_char8();
	rv = (void *)new String((char *)v.export_char8());
	break;
    default:
	THROW(dhdferr_datatype);
    }
    return rv;
}

void *ExportDataForDODS(const hdf_genvec& v, int i) {
    void *rv;			// reminder: rv is single value, must be
				// deleted with delete, not delete[]
    switch (v.number_type()) {
    case DFNT_INT8:
    case DFNT_INT16:
    case DFNT_INT32:
	rv = (void *)new int32;
	*((int32 *)rv)= v.elt_int32(i);
	break;
    case DFNT_UINT8:
    case DFNT_UINT16:
    case DFNT_UINT32:
	rv = (void *)new uint32;
	*((uint32 *)rv)= v.elt_uint32(i);
	break;
    case DFNT_FLOAT32: 
    case DFNT_FLOAT64: 
	rv = (void *)new float64;
	*((float64 *)rv)= v.elt_float64(i);
	break;
    case DFNT_UCHAR8:
	rv = (void *)new uchar8;
	*((uchar8 *)rv)= v.elt_uchar8(i);
	break;
    case DFNT_CHAR8:
//	rv = (void *)new char8;
//	*((char8 *)rv)= v.elt_char8(i);
	rv = (void *)new String(v.elt_char8(i));
	break;
    default:
	THROW(dhdferr_datatype);
    }
    return rv;
}

HDFStructure *CastBaseTypeToStructure(BaseType *p) {
    if (p->type_name() != "Structure")
	THROW(dhdferr_consist);
    return (HDFStructure *)p;
}

HDFArray *CastBaseTypeToArray(BaseType *p) {
    if (p->type_name() != "Array")
	THROW(dhdferr_consist);
    return (HDFArray *)p;
}

