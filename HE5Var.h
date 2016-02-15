#ifndef _HE5Var_H
#define _HE5Var_H

#include <vector>
#include "HE5Dim.h"

using namespace std;

struct HE5Var {
    string name;
    vector<HE5Dim> dim_list;
    // UNCOMMENT OUT the line below to retrieve the maximum dimension list. ALSO NEED TO ADD MAX_DIMENSION_LIST at  he5dds.lex
    //vector<HE5Dim> max_dim_list;
};

#endif
