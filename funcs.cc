#include <vector.h>
#include <String.h>

#include <BaseType.h>
#include <HDFArray.h>
#include <DDS.h>

BaseType *test_func(int argc, BaseType *argv[], DDS &);
// BaseType *subset_geo_func(int argc, BaseType *argv[], DDS &);

void register_funcs(DDS& dds) {
//  dds.add_function("subset_geo", subset_geo_func);
    dds.add_function("test", test_func);
}


//
// implements a No-op function to test adding functions to the HDF server
BaseType *test_func(int argc, BaseType *argv[], DDS &) {

// Check Usage
    if (argc != 1) {
	cerr << "Usage: test(var)" << endl;
	return 0;
    }

    return argv[0];
}

#ifdef 0
//
// implements subset_geo(time1,time2,lat1,lon1,lat2,lon2,var) for SwathArray
BaseType *subset_geo_func(int argc, BaseType *argv[], DDS &) {

// Check Usage
    if (argc != 7) {
	cerr << "Usage: subset_geo(time1,time2,lat1,lon1,lat2,lon2,var)" 
	     << endl;
	return 0;
    }

    // Make sure variable is an Array -- later we'll modify this routine to 
    // work with other DODS type
    if (argv[7]->type_name() != "Array") {
	cerr << "This subsetting routine works only on Arrays" << endl;
	return 0;
    }

    // - Get name of datafile and dataset (how? - DDS only gives datafile name)
    // - Look up dataset directory in site catalog list
    // - Instantiate appropriate Dataset subclass from .hld file
    // - If SwathArray Dataset:
    //   * Invoke get_tracks to identify tracks of subset
    //   * Extract data from DODS and return
    
    return argv[7];
}

#endif
