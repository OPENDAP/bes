
#include "config_hdf.h"

// libg++
#include <iostream>
#include <vector>
#include <string>

using std::cerr ;
using std::endl ;

// DODS
#include <BaseType.h>
#include <HDFArray.h>
#include <DDS.h>

#define MASTER_CONF "./master.conf" // yuck, yes I know

BaseType *test_func(int argc, BaseType *argv[], DDS &);

void register_funcs(DDS& dds) {
    dds.add_function("test", test_func);
}

// implements a No-op function to test adding functions to the HDF server

BaseType *test_func(int argc, BaseType *argv[], DDS &) {
    // Check Usage
    if (argc != 1) {
	cerr << "Usage: test(var)" << endl;
	return 0;
    }

    return argv[0];
}

#if 0
//
// implements subset_geo(time1,time2,lat1,lon1,lat2,lon2,var) for SwathArray
BaseType *subset_geo_func(int argc, BaseType *argv[], DDS &dds) {
    
    // Check Usage
    if (argc != 7) {
	cerr 
	  << "Usage: subset_geo(time1,time2,lat1,lon1,lat2,lon2,varname)"
	  << endl;
	return 0;
    }

    // Make sure variables are Arrays -- later we'll modify this routine to 
    // work with other DODS types
    if (argv[7]->type() != dods_array_c) {
	cerr << "This subsetting routine works only on Arrays" << endl;
	return 0;
    }

    ErrorHandler errors;	// access to errors from catalog, dataset code

    // Implement a hack to look up the name of the dataset given the data
    // file name; it gets it from the regex item in the 
    // Master_Catalog::Dataset_Info structure.  The hack is that MASTER_CONF
    // must be defined in the source code; it should be provided somehow by

    // DODS.  This is one of those areas where the seam between the CalVal 
    // code and DODS is not very tight.

    // Access the master catalog
    Master_Catalog mcat(MASTER_CONF);
    if (!mcat || (bool)errors) {
	cerr << "subset_geo has failed: could not open master catalog" << endl;
        return 0;
    }
    vector<Master_Catalog::Dataset_Info> dslist = mcat.dslist();
    
    // find the dataset whose filename regex matches our filename
    Master_Catalog::Dataset_Info dsinfo;
    for (int i=0; i<(int)dslist.size(); ++i) 
	if (dslist[i].dsregex.length() > 0) {
	    if (dds.filename().matches(Regex(dslist[i].dsregex))) {
		dsinfo = dslist[i];
		break;
	    }
	}
    
    if (!dsinfo  ||  (bool)errors) {		// still unitialized
	cerr << "subset_geo has failed: could not lookup dataset" << endl;
	return 0;
    }


    // Now on with the show.  

    // Instantiate appropriate Dataset subclass from .hld file
    string hldfilename = dsinfo.dsname + ".hld";
    Dataset *ds = Dataset::factory(dsinfo.dspath, hldfilename);
    if (!*ds  ||  (bool)errors) {
	cerr << "subset_geo has failed: could not initialize dataset" << endl;
	return 0;
    }

    // Construct geobox, date range from arguments
    
    double mindate = *(CastBaseTypeToDouble(argv[1]));
    double maxdate = *(CastBaseTypeToDouble(argv[2]));
    DateRange dr = DateRange(mindate, maxdate);
    if (!dr) {
	ErrorHandler("Could not parse date arguments");
	return 0;
    }

    Latitude minlat = (float)*(CastBaseTypeToDouble(argv[3]));
    Longitude minlon = (float)*(CastBaseTypeToDouble(argv[4]));
    Latitude maxlat = (float)*(CastBaseTypeToDouble(argv[5]));
    Longitude maxlon = (float)*(CastBaseTypeToDouble(argv[6]));
    GeoBox geobox = GeoBox(GeoCoord(minlat, minlon), GeoCoord(maxlat,maxlon));
    if (!geobox) {
	ErrorHandler("Could not parse spatial region arguments");
	return 0;
    }
    string filename = dsinfo.dspath + "/" + dds.filename();

    // if a SwathArray,
    if (ds->type() == Dataset::swatharray) {

	// look up tracks corresponding to geobox and daterange
	SwathArray *sa = (SwathArray *)ds;
	vector<SwathArray::track_pair> exts = 
	    sa->get_tracks(filename, dr, geobox);
	if (exts.size() > 1) {
	    cerr << "Function cannot currently handle queries that return more than one subset";
	    return 0;
	}
	else if (exts.size() == 0)
	    return 0;		// HoW to do this??

	// formulate constraint for DODS
	Pix p = ((Array *)argv[7])->first_dim();
	((Array *)argv[7])->add_constraint(p,exts[0].first, 1, exts[0].second);
    }

    else {			// if not SwathArray
	cerr << "Function currently cannot handle lookups on datasets types other than SwathArray";
	return 0;
    }
    
    return argv[7];
}
#endif








