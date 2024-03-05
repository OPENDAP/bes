//
// Created by James Gallagher on 11/8/22.
//

#include "config.h"

#include <iostream>
#include <string>
#include <cstring>

#include <sys/stat.h>
#include <unistd.h>

#include "TheBESKeys.h"
#include "RemoteResource.h"
#include "HttpNames.h"
#include "BESError.h"
#include "BESDebug.h"

using namespace std;
using namespace http;

static void usage()
{
    cerr << R"(Usage: remote_resource_tester [-v] [-r expected_size] [-b bes.conf] <url> <debug config>
       -v: verbose
       -r: retry
       -b: bes.conf)" << endl;
}

/**
 * Takes two arguments: bes.conf and a URL which must be usable by RemoteResource
 * No error checking to speak of.
 * @param argc
 * @param argv
 * @return 0 if things work, 10 if some sort of BESError is caught, else EXIT_FAILURE
 */
int main(int argc, char *argv[])
{
    bool verbose = false;
    bool retry = false;
    long expected_size = 0;
    string bes_conf = "bes.conf";

    int option_char;
    while ((option_char = getopt(argc, argv, "r:b:vdh")) != EOF)
        switch (option_char) {
            case 'v':
                verbose = true;
                break;
            case 'r':
                retry = true;
                expected_size = atoi(optarg);
                break;
            case 'b':
                bes_conf = optarg;
                break;
            case 'h':
                usage();
                break;
            default:
                break;
        }

    argc -= optind;
    argv += optind;

    if (argc != 2) {
        usage();
        return EXIT_FAILURE;
    }

    try {
        string debug_dest = argv[1];
        debug_dest.append(",http");
        BESDebug::SetUp(debug_dest);

        if (!bes_conf.empty())
            TheBESKeys::ConfigFile = bes_conf;

        string tmp_dir_path = TheBESKeys::TheKeys()->read_string_key(REMOTE_RESOURCE_TMP_DIR_KEY, "/tmp/bes_rr_tmp");
        mkdir(tmp_dir_path.c_str(), 0777);

        auto url = std::make_shared<http::url>(argv[0]);
        auto rr = std::make_unique<RemoteResource>(url);

        rr->retrieve_resource();

        string name = rr->get_filename();

        if (retry) {
            struct stat statbuf{};
            auto status = stat(name.c_str(), &statbuf);
            if (status != 0) {
                cerr << "stat failed: " << strerror(errno) << endl;
                return EXIT_FAILURE;
            }

            if (verbose)
                cerr << "size: " << statbuf.st_size << ", expected: " <<  expected_size << endl;

            if (statbuf.st_size == 0) {
                cerr << "statbuf.st_size == 0" << endl;
                return EXIT_FAILURE + 12;
            }
            else if (statbuf.st_size != expected_size) {
                cerr << "statbuf.st_size != expected_size" << endl;
                return EXIT_FAILURE + 13;
            }

            return EXIT_SUCCESS;
        }
        else {
            if (verbose)
                cerr << "Data read from: " << name << endl;

            ifstream rr_cache_file(name);
            while (rr_cache_file) {
                string line;
                getline(rr_cache_file, line);
                cout << line << endl;
            }

            return EXIT_SUCCESS;
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
}
