
#ifndef DAP_UNIT_TESTS_TESTFUNCTION_H_
#define DAP_UNIT_TESTS_TESTFUNCTION_H_

#include <Array.h>
#include <Byte.h>
#include <ServerFunctionsList.h>

#include <DAS.h>
#include <DDS.h>
#include <util.h>

//#define KEY "response_cache"

class TestFunction: public libdap::ServerFunction
{
private:
    const static libdap::Type requested_type;// = libdap::dods_byte_c;
    const static int num_dim;// = 2;
    const static int dim_sz;// = 3;

    /**
     * Server function used by the ConstraintEvaluator. This is needed because passing
     * the CE a null expression or one that names an non-existent function is an error.
     *
     * The test harness code must load up the ServerFunctionList instance - see also
     * TestFunction.h
     */
    static void function_dap2_test(int argc, libdap::BaseType *argv[], libdap::DDS &dds, libdap::BaseType **btpp)
    {
        if (argc != 1) {
            throw libdap::Error(malformed_expr, "test(name) requires one argument.");
        }

        std::string name = libdap::extract_string_argument(argv[0]);

        libdap::Array *dest = new libdap::Array(name, 0);   // The ctor for Array copies the prototype pointer...
        libdap::BaseTypeFactory btf;
        dest->add_var_nocopy(btf.NewVariable(requested_type, name));  // ... so use add_var_nocopy() to add it instead

        vector<int> dims(num_dim, dim_sz);
        //dims.push_back(3); dims.push_back(3);
        unsigned long num_elem = 1;
        auto i = dims.begin();
        while (i != dims.end()) {
            num_elem *= *i;
            dest->append_dim(*i++);
        }

        // stuff the array with values
        vector<libdap::dods_byte> values(num_elem);
        for (unsigned int i = 0; i < num_elem; ++i) {
            values[i] = i;
        }

        dest->set_value(values, num_elem);

        dest->set_send_p(true);
        dest->set_read_p(true);

        // return the array
        *btpp = dest;
    }

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

const libdap::Type TestFunction::requested_type = libdap::dods_byte_c;
const int TestFunction::num_dim = 2;
const int TestFunction::dim_sz = 3;

#endif /* DAP_UNIT_TESTS_TESTFUNCTION_H_ */
