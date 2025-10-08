#ifndef _HE5Var_H
#define _HE5Var_H

#include <vector>
#include "HE5Dim.h"


struct HE5Var {
    std::string name;
    std::vector<HE5Dim> dim_list;
    std::vector<HE5Dim> max_dim_list;
};

#endif
