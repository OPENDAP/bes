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
// Revision 1.1  1996/10/31 18:43:02  jimg
// Added.
//
//
//////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif
#include <vector.h>
#include <mfhdf.h>

vector<String> split(const String& str, const String& delim) {
    vector<String> rv;

    int len = str.length();
    int dlen = delim.length();
    for (int i=0, previ=-dlen; ;previ = i) {
#ifdef __GNUG__
	i = str.index(delim, previ+dlen);
#else
	i = str.find(delim, previ+dlen);
#endif
	if (i == 0)
	    continue;
	if (i < 0) {
	    if (previ+dlen < len)
#ifdef __GNUG__
		rv.push_back(((String)str).at(previ+dlen,(len-previ-dlen)));
#else
	        rv.push_back(str.substr(previ+dlen,(len-previ-dlen)));
#endif
	    break;
	}
#ifdef __GNUG__
	rv.push_back(((String)str).at(previ+dlen,(i-previ-dlen)));
#else
	rv.push_back(str.substr(previ+dlen,(i-previ-dlen)));
#endif
    }

    return rv;
}

String join(const vector<String>& sv, const String& delim) {
    String str;
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


