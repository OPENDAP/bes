//
// Created by James Gallagher on 11/8/22.
//

#include "config.h"

#include <iostream>
#include <string>

#include <sys/stat.h>

#include "TheBESKeys.h"
#include "RemoteResource.h"
#include "HttpNames.h"
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
int main(int, char *argv[])
{
    try {
        string debug_dest = argv[3];
        debug_dest.append(",http");
        BESDebug::SetUp(debug_dest);
        TheBESKeys::ConfigFile = argv[1];

        string tmp_dir_path = TheBESKeys::TheKeys()->read_string_key(REMOTE_RESOURCE_TMP_DIR_KEY, "/tmp/bes_rr_tmp");
        mkdir("/tmp/bes_rr_tmp", 0777);

        auto url = std::make_shared<http::url>(argv[2]);
        auto rr = std::make_unique<RemoteResource>(url);

        rr->retrieve_resource();

        string name = rr->get_filename();

        ifstream rr_cache_file(name);
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
