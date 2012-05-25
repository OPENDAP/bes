#ifndef _Grid_H
#define _Grid_H

#include <string>
#include <vector>
#include <iostream>

using namespace std;

#include "HE5GridPara.h"
#include "HE5Var.h"

struct HE5Grid {

    string name;
    vector<HE5Dim> dim_list;
    vector<HE5Var> data_var_list;

    /// The bottom coordinate value of a Grid
    float point_lower;
    /// The top coordinate value of a Grid
    float point_upper;
    /// The leftmost coordinate value of a Grid
    float point_left;
    /// The rightmost coordinate value of a Grid
    float point_right;

    // The following pixel registration, grid origin, and projection code
    // are defined in include/HE5_HdfEosDef.h that can be found in 
    // HDF-EOS5 library distribution. 

    // PixelRegistration
    // These are actually EOS5 constants, but we define these
    // since we do not depend on the HDF-EOS5 lib.
     EOS5GridPRType pixelregistration; // either _HE5_HDFE_(CENTER|CORNER)


    // GridOrigin
    EOS5GridOriginType gridorigin; // one of HE5_HDFE_GD_(U|L)(L|R)

    // ProjectionCode
    EOS5GridPCType projection;

};
#endif
