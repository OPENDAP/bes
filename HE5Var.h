#ifndef _HE5Var_H
#define _HE5Var_H

#include <vector>
#include "HE5Dim.h"

using namespace std;

struct HE5Var {
    string name;
    vector<HE5Dim> dim_list;
};

#endif
