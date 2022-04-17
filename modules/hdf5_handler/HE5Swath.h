#ifndef _HE5Swath_H
#define _HE5Swath_H
#include "HE5Var.h"


struct HE5Swath {

 public:
    std::string name;
    std::vector<HE5Dim> dim_list;
    std::vector<HE5Var> geo_var_list;
    std::vector<HE5Var> data_var_list;
};

#endif
