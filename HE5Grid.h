#ifndef _Grid_H
#define _Grid_H

#include <string>
#include <vector>
#include <iostream>


#include "HE5GridPara.h"
#include "HE5Var.h"

struct HE5Grid {

    std::string name;
    std::vector<HE5Dim> dim_list;
    std::vector<HE5Var> data_var_list;

    /// The bottom coordinate value of a Grid
    double point_lower;
    //float  point_lower;
    /// The top coordinate value of a Grid
    double point_upper;
    /// The leftmost coordinate value of a Grid
    double point_left;
    /// The rightmost coordinate value of a Grid
    double point_right;

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

    // Projection parameters
    double param[13];

    // zone (may only be applied to UTM)
    int zone;

    // sphere
    int sphere;

};
#endif
