//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (c) 1996, California Institute of Technology.
//                     U.S. Government Sponsorship under NASA Contract
//		       NAS7-1260 is acknowledged.
// 
// Author: Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: hcutil.cc,v $ - misc utility routines for HDFClass
//
// $Log: hcutil.cc,v $
// Revision 1.4  2000/03/09 01:44:33  jimg
// merge with 3.1.3
//
// Revision 1.3.8.1  2000/03/09 00:24:59  jimg
// Replaced int and uint32 with string::size_type
//
// Revision 1.3  1999/05/06 03:23:33  jimg
// Merged changes from no-gnu branch
//
// Revision 1.2  1999/05/05 23:33:43  jimg
// String --> string conversion
//
// Revision 1.1.20.1  1999/05/06 00:35:45  jimg
// Jakes String --> string changes
//
// Revision 1.1  1996/10/31 18:43:02  jimg
// Added.
//
//
//////////////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>

#include <mfhdf.h>

vector<string> split(const string& str, const string& delim) {
    vector<string> rv;

    string::size_type len = str.length();
    string::size_type dlen = delim.length();
    for (string::size_type i=0, previ=-dlen; ;previ = i) {
	i = str.find(delim, previ+dlen);
	if (i == 0)
	    continue;
	if (i < 0) {
	    if (previ+dlen < len)
	        rv.push_back(str.substr(previ+dlen,(len-previ-dlen)));
	    break;
	}
	rv.push_back(str.substr(previ+dlen,(i-previ-dlen)));
    }

    return rv;
}

string join(const vector<string>& sv, const string& delim) {
    string str;
    if (sv.size() > 0) {
	str = sv[0];
	for (int i=1; i<(int)sv.size(); ++i)
	    str += (delim + sv[i]);
    }
    return str;
}

bool SDSExists(const char *filename, const char *sdsname) {

    int32 sd_id, index;
    if ( (sd_id = SDstart(filename, DFACC_RDONLY)) < 0)
	return false;
    
    index = SDnametoindex(sd_id, (char *)sdsname);
    SDend(sd_id);

    return (index >= 0);
}

bool GRExists(const char *filename, const char *grname) {

    int32 file_id, gr_id, index;
    if ( (file_id = Hopen(filename, DFACC_RDONLY, 0)) < 0)
	return false;
    if ( (gr_id = GRstart(file_id)) < 0)
	return false;
    
    index = GRnametoindex(gr_id, (char *)grname);
    GRend(gr_id);
    Hclose(file_id);

    return (index >= 0);
}

bool VdataExists(const char *filename, const char *vdname) {

    int32 file_id, ref;
    if ( (file_id = Hopen(filename, DFACC_RDONLY, 0)) < 0)
	return false;
    Vstart(file_id);
    ref = VSfind(file_id, vdname);
    Vend(file_id);
    Hclose(file_id);
    
    return (ref > 0);
}


