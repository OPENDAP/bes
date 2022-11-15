//
// Created by James Gallagher on 11/8/22.
//

#include "config.h"

#include <iostream>
#include <fstream>
#include <string>

#include "TheBESKeys.h"
#include "RemoteResource.h"
#include "url_impl.h"
#include "BESError.h"
#include "BESDebug.h"

using namespace std;
using namespace http;

/**
 * Takes two arguments: bes.conf and a URL which must be usable by RemoteResource
 * No error checking to speak of.
 * @param argc
 * @param argv
 * @return 0 if things work, 10 if some sort of BESError is caught, else EXIT_FAILURE
 */
int main(int argc, char *argv[])
{
    cerr << "Hello, world!" << endl;
    string bes_conf = "bes.conf";

    try {
        string debug_dest = argv[3];
        debug_dest.append(",http");
        BESDebug::SetUp(debug_dest);
        TheBESKeys::ConfigFile = argv[1];
        shared_ptr<http::url> url(new http::url(argv[2]));

        unique_ptr<RemoteResource> rr(new RemoteResource(url));

        rr->retrieveResource();

        string name = rr->getCacheFileName();

        ifstream rr_cache_file = ifstream(name);
        while (rr_cache_file) {
            string line;
            getline(rr_cache_file, line);
            cout << line << endl;
        }
    }
    catch (const BESError &e) {
        cerr << "BES Exception: " << e.get_verbose_message() << endl;
        return EXIT_FAILURE + 10;
    }
    catch (const std::exception &e) {
        cerr << "STD Exception: " << e.what() << endl;
        return EXIT_FAILURE + 11;
    }
    catch (...) {
        cerr << "Unknown exception" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
