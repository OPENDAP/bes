#ifndef _HE5Swath_H
#define _HE5Swath_H
#include "HE5Var.h"

using namespace std;

struct HE5Swath {

 public:
    string name;
    vector<HE5Dim> dim_list;
    vector<HE5Var> geo_var_list;
    vector<HE5Var> data_var_list;
};

#endif
