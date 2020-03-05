//
// Created by ndp on 3/4/20.
//


#include <BESError.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <cerrno>


#include "StandAloneApp.h"

int main(int argc, char **argv)
{
    try {
        StandAloneApp app;
        return app.main(argc, argv);
    }
    catch (BESError &e) {
        std::cerr << "Caught BES Error while starting the command processor: " << e.get_message() << std::endl;
        return 1;
    }
    catch (std::exception &e) {
        std::cerr << "Caught C++ error while starting the command processor: " << e.what() << std::endl;
        return 2;
    }
    catch (...) {
        std::cerr << "Caught unknown error while starting the command processor." << std::endl;
        return 3;
    }
}
