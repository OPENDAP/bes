
#ifndef DAP_UNIT_TESTS_TESTFUNCTION_H_
#define DAP_UNIT_TESTS_TESTFUNCTION_H_

#include <Array.h>
#include <Byte.h>
#include <ServerFunctionsList.h>

#include <DAS.h>
#include <DDS.h>
#include <util.h>

#include "BESInternalFatalError.h"

namespace functions {

    class TestFunction : public libdap::ServerFunction {
    private:
        const static libdap::Type requested_type;// = libdap::dods_byte_c;
        const static int num_dim;// = 2;
        const static int dim_sz;// = 3;

        /**
         * Test function that will return a simple array. The resulting variable
         * will have its attributes set to the attributes in the first var of the
         * DDS passed to the function. This is part of the
         * refactoring that has changed the Data response so that it always holds
         * the DAS as well. We need to be sure that the code bundle attributes into
         * the DDS when functions are called works.
         */
        static void function_dap2_test(int argc, libdap::BaseType *argv[], libdap::DDS &dds, libdap::BaseType **btpp) {
            if (argc != 1) {
                throw libdap::Error(malformed_expr, "test(name) requires one argument.");
            }

            std::string name = libdap::extract_string_argument(argv[0]);

            libdap::Array *dest = new libdap::Array(name, 0);   // The ctor for Array copies the prototype pointer...
            libdap::BaseTypeFactory btf;
            dest->add_var_nocopy(
                    btf.NewVariable(requested_type, name));  // ... so use add_var_nocopy() to add it instead

            vector<int> dims(num_dim, dim_sz);
            unsigned int num_elem = 1;
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

            libdap::AttrTable attr = (*dds.var_begin())->get_attr_table();
            if (attr.get_size() == 0)
                throw BESInternalFatalError("Expected to find an AttrTable object in DDS passed to the test function", __FILE__, __LINE__);
            dest->set_attr_table(attr);

            dest->set_send_p(true);
            dest->set_read_p(true);

            // return the array
            *btpp = dest;
        }

    public:
        TestFunction() {
            setName("test");
            setDescriptionString("The test() function returns a new array.");
            setUsageString("test()");
            setRole("http://services.opendap.org/dap4/server-side-function/");
            setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions");
            setFunction(TestFunction::function_dap2_test);
            // setFunction(TestFunction::function_dap4_tabular);
            setVersion("1.0");
        }

        virtual ~TestFunction() {
        }
    };

    const libdap::Type TestFunction::requested_type = libdap::dods_byte_c;
    const int TestFunction::num_dim = 2;
    const int TestFunction::dim_sz = 3;

} // namespace functions

#endif /* DAP_UNIT_TESTS_TESTFUNCTION_H_ */
