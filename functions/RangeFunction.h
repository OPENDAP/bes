/*
 * RangeFunction.h
 *
 *  Created on: Jun 8, 2016
 *      Author: ndp
 */

#ifndef FUNCTIONS_RANGEFUNCTION_H_
#define FUNCTIONS_RANGEFUNCTION_H_

#include <ServerFunction.h>

namespace libdap {
class BaseType;
class DDS;
}

namespace functions {

/**
 * The function_dap2_range() function determines the range of the passed variable and returns it in a DAP2 array of size two
 * such that:
 * result[0] = minimum value
 * result[1] = maximum value
 *
 * The function accepts a mandatory first parameter, the variable whose range will be determined.
 * The function also accepts an optional second parameter that provides the value of "MISSING VALUE"
 * which will cause the passed value to be ignored in the computation of range.
 */
void function_dap2_range(int argc, libdap::BaseType *argv[], libdap::DDS &dds, libdap::BaseType **btpp) ;

/**
 * The function_dap4_range() function determines the range of the passed variable and returns it in a DAP4 array of size two
 * such that:
 * result[0] = minimum value
 * result[1] = maximum value
 *
 * The function accepts a mandatory first parameter, the variable whose range will be determined.
 * The function also accepts an optional second parameter that provides the value of "MISSING VALUE"
 * which will cause the passed value to be ignored in the computation of range.
 */
libdap::BaseType *function_dap4_range(libdap::D4RValueList *args, libdap::DMR &dmr);

/**
 * The RangeFunction class encapsulates the server side function 'range()'
 * along with additional meta-data regarding its use and applicability.
 */
class RangeFunction: public libdap::ServerFunction {
public:
    RangeFunction()
    {
        setName("range");
        setDescriptionString("The range() function evaluates the passed variable and returns an array of size 2 containing the min and max values of the variable.");
        setUsageString("range(var)");
        setRole("http://services.opendap.org/dap4/server-side-function/range");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#range");
        setFunction(function_dap2_range);
        setFunction(function_dap4_range);
        setVersion("1.0b1");
    }
    virtual ~RangeFunction()
    {
    }
};

} // functions namespace

#endif /* FUNCTIONS_RANGEFUNCTION_H_ */
