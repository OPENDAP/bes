#ifndef _HE5Var_H
#define _HE5Var_H

#include <vector>
#include "HE5Dim.h"


struct HE5Var {
    std::string name;
    std::vector<HE5Dim> dim_list;
    // UNCOMMENT OUT the line below to retrieve the maximum dimension list. ALSO NEED TO ADD MAX_DIMENSION_LIST at  he5dds.lex
//#if 0
    std::vector<HE5Dim> max_dim_list;
//#endif
};

#endif
