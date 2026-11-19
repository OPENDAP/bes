// GeotiffToDmrMain.cc
//
// main() for the GeotiffToDmr tool. The work is done in GeotiffToDmr.cc.

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>

#include "GeotiffToDmr.h"

using namespace geotiff_to_dmr;

int main(int argc, char **argv) {
    CmdArgs args;
    try {
        args = parse_args(argc, argv);
    } catch (const GeoToDMRInternalError &e) {
        std::cerr << "GeotiffToDmr: " << e.what() << '\n';
        return e.code;
    }
    g_verbose = args.verbose;

    std::string bes_exe = which(args.besstandalone);
    if (bes_exe.empty()) {
        std::cerr << "GeotiffToDmr: besstandalone not found ('" << args.besstandalone
                  << "'); install the BES or pass --besstandalone PATH\n";
        return kInternal;
    }
    std::string lib_dir;
    try {
        lib_dir = find_bes_lib_dir(args, bes_exe);
    } catch (const GeoToDMRInternalError &e) {
        std::cerr << "GeotiffToDmr: " << e.what() << '\n';
        return e.code;
    }
    if (!lib_dir.empty()) log_verbose("BES modules: " + lib_dir);

    int worst = kOk;
    int ok = 0;
    for (const auto &input : args.inputs) {
        try {
            process_one(args, input, bes_exe, lib_dir);
            ++ok;
        } catch (const GeoToDMRInternalError &e) {
            std::cerr << "GeotiffToDmr: " << e.what() << '\n';
            worst = std::max(worst, static_cast<int>(e.code));
        } catch (const std::exception &e) {
            std::cerr << "GeotiffToDmr: " << input << ": internal error: " << e.what() << '\n';
            worst = std::max(worst, static_cast<int>(kInternal));
        }
    }

    if (args.inputs.size() > 1)
        std::cerr << "GeotiffToDmr: " << ok << " of " << args.inputs.size() << " files converted\n";
    return worst;
}
