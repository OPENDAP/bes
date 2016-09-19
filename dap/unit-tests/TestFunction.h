/*
 * TestFunction.h
 *
 *  Created on: Sep 19, 2016
 *      Author: jimg
 */

#ifndef DAP_UNIT_TESTS_TESTFUNCTION_H_
#define DAP_UNIT_TESTS_TESTFUNCTION_H_


class TestFunction: public libdap::ServerFunction
{
private:
    static void function_dap2_test(int argc, libdap::BaseType *argv[], libdap::DDS &dds, libdap::BaseType **btpp);

public:
    TestFunction()
    {
        setName("test");
        setDescriptionString("The test() function returns a new array.");
        setUsageString("test()");
        setRole("http://services.opendap.org/dap4/server-side-function/");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions");
        setFunction(TestFunction::function_dap2_test);
        // setFunction(TestFunction::function_dap4_tabular);
        setVersion("1.0");
    }

    virtual ~TestFunction()
    {
    }
};



#endif /* DAP_UNIT_TESTS_TESTFUNCTION_H_ */
