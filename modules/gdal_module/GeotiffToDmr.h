// GeotiffToDmr.h
//
// Declarations for the GeotiffToDmr tool, which returns the DAP4 DMR for a
// GeoTIFF by running besstandalone with the BES GDAL handler. See
// GeotiffToDmr.cc for how a file is processed. Everything is declared here
// so the functions can be unit-tested (unit-tests/GeotiffToDmrTest.cc).

#ifndef GEOTIFF_TO_DMR_H_
#define GEOTIFF_TO_DMR_H_

#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace geotiff_to_dmr {

extern const char *const kVersion;

// ---------------------------------------------------------------------------
// Exit codes and errors
// ---------------------------------------------------------------------------
enum ExitCode : int {
    kOk = 0,
    kBadInput = 1,
    kBesError = 2,
    kTimeout = 3,
    kInternal = 4,
};

struct GeoToDMRInternalError : std::runtime_error {
    ExitCode code;
    GeoToDMRInternalError(ExitCode c, const std::string &msg) : std::runtime_error(msg), code(c) {}
};

// ---------------------------------------------------------------------------
// CmdArgs
// ---------------------------------------------------------------------------
struct CmdArgs {
    std::vector<std::string> inputs;
    std::string output;            // -o: single input only; "-" = stdout
    std::string out_dir;           // -d: one <name>.dmr per input
    std::string besstandalone;     // path or name on PATH
    std::string bes_lib_dir;       // dir holding lib*_module.so
    std::string bes_conf;          // optional base config to BES.Include
    std::vector<std::string> extra_keys;  // KEY=VALUE lines appended to bes.conf
    int timeout_sec = 60;
    bool keep_temp = false;
    bool verbose = false;
};

extern bool g_verbose;

// Logging and usage
void log_verbose(const std::string &msg);
void print_usage(std::ostream &os);

// Command line. Prints usage and calls std::exit(0) for -h/--help and
// --version; throws GeoToDMRInternalError(kBadInput) for usage errors.
CmdArgs parse_args(int argc, char **argv);

// Path and file helpers (POSIX)
std::string path_join(const std::string &a, const std::string &b);
std::string path_dirname(const std::string &p);
std::string path_basename(const std::string &p);
std::string path_extension_lower(const std::string &p);
bool path_exists(const std::string &p);
std::string path_absolute(const std::string &p);
void make_dirs(const std::string &dir);
void remove_tree(const std::string &dir);
std::string read_file(const std::string &p);
void write_file(const std::string &p, const std::string &content);
std::string xml_escape(const std::string &s);
std::string trim(const std::string &s);
std::string which(const std::string &prog);

// RAII temp directory under $TMPDIR (default /tmp); removed on destruction
// unless keep is true.
class TempDir {
public:
    explicit TempDir(bool keep);
    ~TempDir();
    TempDir(const TempDir &) = delete;
    TempDir &operator=(const TempDir &) = delete;
    const std::string &path() const { return path_; }

private:
    std::string path_;
    bool keep_;
};

// Step 1: input validation. Returns the canonical path.
std::string validate_input(const std::string &input);

// Step 2: BES configuration and command file
std::string find_bes_lib_dir(const CmdArgs &args, const std::string &bes_exe);
std::string make_bes_conf(const CmdArgs &args, const std::string &tmp, const std::string &root,
                          const std::string &lib_dir);
std::string make_bescmd(const std::string &file_in_root);

// Step 3: run besstandalone with a timeout
struct RunResult {
    int exit_status = -1;   // valid if !signaled
    bool signaled = false;
    int signal = 0;
    bool timed_out = false;
    std::string out;
    std::string err;
};

RunResult run_besstandalone(const std::string &exe, const std::string &conf,
                            const std::string &cmd, const std::string &tmp, int timeout_sec);

// Step 4: interpret the response
std::string extract_tag(const std::string &xml, const std::string &tag);
std::string interpret_response(const RunResult &r, const std::string &input, int timeout_sec);

// Per-file driver
std::string output_path_for(const CmdArgs &args, const std::string &input);
void write_output(const std::string &dest, const std::string &dmr);
void process_one(const CmdArgs &args, const std::string &input, const std::string &bes_exe,
                 const std::string &lib_dir);

}  // namespace geotiff_to_dmr

#endif  // GEOTIFF_TO_DMR_H_
