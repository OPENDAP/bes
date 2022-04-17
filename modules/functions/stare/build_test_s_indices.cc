//
// Created by James Gallagher on 7/1/20.
//
/**

  Make a circular region of interest and a 2D geolocated array, then
  use STARE SpatialRange to determine what points of the array are in
  the ROI.

   Subsetting: Given a 2D data array, where the level 27 sindex is
   known for each element and a set of sindices that define a ROI,
   return the list of indices in the array that fall within the ROI
   (effectively, this is region intersection). What qualifications
   should be placed on the two sets of indices to improve performance?

 */

#include <iostream>
#include <unistd.h>
#include <STARE.h>
#include <SpatialRange.h>

using namespace std;

void usage(const char *name) {
    cerr << name << " [-v -c -a <lat center> -o <lon center> -r <radius> -l <level>] | -h" << endl;
}

int main(int argc, char *argv[]) {

    int c;
    extern char *optarg;
    extern int optind;
    bool verbose = false;
    float64 latDegrees = 0.0, lonDegrees = 0.0, radius_degrees = 0.5;
    int force_resolution_level = 10; // 10km neighborhood
    bool csv = false;

    while ((c = getopt(argc, argv, "vhca:o:r:l:")) != -1) {
        switch (c) {
            case 'v':
                verbose = true;
                break;
            case 'o':
                latDegrees = atof(optarg);
                break;
            case 'a':
                lonDegrees = atof(optarg);
                break;
            case 'r':
                radius_degrees = atof(optarg);
                break;
            case 'l':
                force_resolution_level = atoi(optarg);
                break;
            case 'c':
                csv = true;
                break;
            case 'h':
            default:
                usage(argv[0]);
                exit(EXIT_SUCCESS);
                break;
        }
    }

    // This resets argc and argv once the optional arguments have been processed. jhrg 12/5/19
    argc -= optind;
    argv += optind;

    STARE stare;

    // Make a region of interest. At thiks writing, a 0.5 degree circle centered at the origin.

    STARE_SpatialIntervals circle_indices = stare.CoverCircleFromLatLonRadiusDegrees(latDegrees, lonDegrees,
                                                                                     radius_degrees,
                                                                                     force_resolution_level);
    if (csv) {
        for (STARE_SpatialIntervals::iterator i = circle_indices.begin(), e = circle_indices.end()-1; i != e; ++i)
            cout << *i << ", ";
        cout << *(circle_indices.end()-1) << endl;
    }
    else {
        for (STARE_ArrayIndexSpatialValue s: circle_indices) {
            cout << s << endl;
        }
    }
}
