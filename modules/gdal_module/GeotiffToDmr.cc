// GeotiffToDmr.cc
//
// Generate the DAP4 DMR for a GeoTIFF using the OPeNDAP BES GDAL handler
// (bes/modules/gdal_module), by driving `besstandalone` (roadmap Option A).
//
// For each input file the tool:
//   1. Validates the input (exists, regular, readable, TIFF/BigTIFF magic;
//      with -DGEOTIFFTODMR_WITH_GDAL it also opens it with GDAL and checks the
//      driver is GTiff and that it is georeferenced).
//   2. Writes a per-run temp bes.conf (catalog root = the file's folder,
//      TypeMatch .tif/.tiff -> gdal) and a "get dmr" .bescmd command file.
//   3. Runs `besstandalone -c bes.conf -i get_dmr.bescmd` with a timeout,
//      capturing stdout/stderr.
//   4. Detects BES error responses, checks the response is a DAP4 DMR, and
//      writes it atomically to <input>.dmr (or -o / --out-dir / stdout).
//
// Exit codes: 0 ok, 1 bad input/usage, 2 BES error, 3 timeout, 4 internal.
//
// Built by modules/gdal_module/Makefile.am. The code is C++14 (the BES
// configure script forces --std=c++14) and POSIX-only. Stand-alone build:
//     g++ -std=c++14 -O2 -o GeotiffToDmr GeotiffToDmr.cc
//
// Compile-time defaults (set by Makefile.am to the install locations):
//     GEOTIFFTODMR_DEFAULT_BESSTANDALONE  path of besstandalone
//     GEOTIFFTODMR_DEFAULT_BES_LIB_DIR    directory holding lib*_module.so
//
// NOTE: BES configuration key names below follow the stock Hyrax bes.conf /
// gdal.conf. Use --bes-conf or --key to adjust for a particular install.

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <climits>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <ftw.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef GEOTIFFTODMR_WITH_GDAL
#include <cpl_error.h>
#include <gdal.h>
#endif

#ifndef GEOTIFFTODMR_DEFAULT_BESSTANDALONE
#define GEOTIFFTODMR_DEFAULT_BESSTANDALONE "besstandalone"
#endif

#ifndef GEOTIFFTODMR_DEFAULT_BES_LIB_DIR
#define GEOTIFFTODMR_DEFAULT_BES_LIB_DIR ""
#endif

namespace {

const char *const kVersion = "0.1.0";

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

struct CustomError : std::runtime_error {
    ExitCode code;
    CustomError(ExitCode c, const std::string &msg) : std::runtime_error(msg), code(c) {}
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

bool g_verbose = false;

void log_verbose(const std::string &msg) {
    if (g_verbose) std::cerr << "GeotiffToDmr: " << msg << '\n';
}

void print_usage(std::ostream &os) {
    os << "Usage: GeotiffToDmr [CmdArgs] INPUT.tif [INPUT2.tif ...]\n"
          "\n"
          "Generate the DAP4 DMR for GeoTIFF files using the OPeNDAP BES GDAL handler.\n"
          "\n"
          "Output (default: <INPUT>.dmr next to each input):\n"
          "  -o, --output FILE         Output file for a single input ('-' = stdout)\n"
          "  -d, --out-dir DIR         Write <name>.dmr for each input into DIR\n"
          "\n"
          "BES:\n"
          "      --besstandalone PATH  besstandalone executable (default: $BESSTANDALONE,\n"
          "                            else " GEOTIFFTODMR_DEFAULT_BESSTANDALONE ")\n"
          "      --bes-lib-dir DIR     Directory with libdap_module.so, libdap_xml_module.so,\n"
          "                            libgdal_module.so (default: auto-detect)\n"
          "      --bes-conf FILE       Base BES config to include (e.g. /etc/bes/bes.conf);\n"
          "                            catalog root and TypeMatch are still overridden\n"
          "      --key KEY=VALUE       Extra bes.conf line (repeatable)\n"
          "      --timeout SEC         Per-file timeout (default: 60)\n"
          "\n"
          "Other:\n"
          "      --keep-temp           Keep the temp bes.conf/.bescmd/logs for debugging\n"
          "  -v, --verbose             Log progress to stderr\n"
          "  -h, --help                Show this help\n"
          "      --version             Show version\n"
          "\n"
          "Exit codes: 0 ok, 1 bad input/usage, 2 BES error, 3 timeout, 4 internal.\n";
}

CmdArgs parse_args(int argc, char **argv) {
    CmdArgs args;
    auto need_value = [&](int &i, const std::string &flag) -> std::string {
        if (i + 1 >= argc) throw CustomError(kBadInput, "missing value for " + flag);
        return argv[++i];
    };

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") {
            print_usage(std::cout);
            std::exit(kOk);
        } else if (a == "--version") {
            std::cout << "GeotiffToDmr " << kVersion << '\n';
            std::exit(kOk);
        } else if (a == "-o" || a == "--output") {
            args.output = need_value(i, a);
        } else if (a == "-d" || a == "--out-dir") {
            args.out_dir = need_value(i, a);
        } else if (a == "--besstandalone") {
            args.besstandalone = need_value(i, a);
        } else if (a == "--bes-lib-dir") {
            args.bes_lib_dir = need_value(i, a);
        } else if (a == "--bes-conf") {
            args.bes_conf = need_value(i, a);
        } else if (a == "--key") {
            std::string kv = need_value(i, a);
            if (kv.find('=') == std::string::npos)
                throw CustomError(kBadInput, "--key expects KEY=VALUE, got: " + kv);
            args.extra_keys.push_back(kv);
        } else if (a == "--timeout") {
            std::string v = need_value(i, a);
            try {
                args.timeout_sec = std::stoi(v);
            } catch (...) {
                throw CustomError(kBadInput, "invalid --timeout: " + v);
            }
            if (args.timeout_sec <= 0) throw CustomError(kBadInput, "--timeout must be > 0");
        } else if (a == "--keep-temp") {
            args.keep_temp = true;
        } else if (a == "-v" || a == "--verbose") {
            args.verbose = true;
        } else if (a == "--") {
            for (++i; i < argc; ++i) args.inputs.emplace_back(argv[i]);
        } else if (!a.empty() && a[0] == '-' && a != "-") {
            throw CustomError(kBadInput, "unknown option: " + a);
        } else {
            args.inputs.push_back(a);
        }
    }

    if (args.inputs.empty()) throw CustomError(kBadInput, "no input file given (see --help)");
    if (!args.output.empty() && args.inputs.size() > 1)
        throw CustomError(kBadInput, "-o/--output works with a single input; use --out-dir");
    if (!args.output.empty() && !args.out_dir.empty())
        throw CustomError(kBadInput, "use either -o/--output or --out-dir, not both");

    if (args.besstandalone.empty()) {
        const char *env = std::getenv("BESSTANDALONE");
        args.besstandalone = (env && *env) ? env : GEOTIFFTODMR_DEFAULT_BESSTANDALONE;
    }
    return args;
}

// ---------------------------------------------------------------------------
// Path and file helpers (POSIX; no std::filesystem, which needs C++17)
// ---------------------------------------------------------------------------
std::string path_join(const std::string &a, const std::string &b) {
    if (a.empty()) return b;
    if (a.back() == '/') return a + b;
    return a + "/" + b;
}

std::string path_dirname(const std::string &p) {
    auto pos = p.find_last_of('/');
    if (pos == std::string::npos) return ".";
    if (pos == 0) return "/";
    return p.substr(0, pos);
}

std::string path_basename(const std::string &p) {
    auto pos = p.find_last_of('/');
    return pos == std::string::npos ? p : p.substr(pos + 1);
}

std::string path_extension_lower(const std::string &p) {
    std::string base = path_basename(p);
    auto pos = base.find_last_of('.');
    if (pos == std::string::npos || pos == 0) return {};
    std::string ext = base.substr(pos);
    for (auto &c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext;
}

bool path_exists(const std::string &p) {
    struct stat st;
    return ::stat(p.c_str(), &st) == 0;
}

// Absolute path for a file that may not exist yet (no symlink resolution).
std::string path_absolute(const std::string &p) {
    if (!p.empty() && p[0] == '/') return p;
    char cwd[PATH_MAX];
    if (!::getcwd(cwd, sizeof cwd))
        throw CustomError(kInternal, std::string("getcwd failed: ") + std::strerror(errno));
    return path_join(cwd, p);
}

void make_dirs(const std::string &dir) {
    if (dir.empty() || path_exists(dir)) return;
    make_dirs(path_dirname(dir));
    if (::mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST)
        throw CustomError(kInternal, "cannot create " + dir + ": " + std::strerror(errno));
}

int nftw_remove(const char *path, const struct stat *, int, struct FTW *) {
    return ::remove(path);
}

void remove_tree(const std::string &dir) {
    ::nftw(dir.c_str(), nftw_remove, 16, FTW_DEPTH | FTW_PHYS);
}

std::string read_file(const std::string &p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void write_file(const std::string &p, const std::string &content) {
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    if (!out) throw CustomError(kInternal, "cannot write " + p);
    out << content;
    if (!out) throw CustomError(kInternal, "error writing " + p);
}

std::string xml_escape(const std::string &s) {
    std::string r;
    r.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&': r += "&amp;"; break;
            case '<': r += "&lt;"; break;
            case '>': r += "&gt;"; break;
            case '"': r += "&quot;"; break;
            case '\'': r += "&apos;"; break;
            default: r += c;
        }
    }
    return r;
}

std::string trim(const std::string &s) {
    const char *ws = " \t\r\n";
    auto b = s.find_first_not_of(ws);
    if (b == std::string::npos) return {};
    auto e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

// Resolve a program name against PATH (returns empty if not found).
std::string which(const std::string &prog) {
    if (prog.find('/') != std::string::npos)
        return (::access(prog.c_str(), X_OK) == 0) ? prog : std::string{};
    const char *path = std::getenv("PATH");
    if (!path) return {};
    std::stringstream ss(path);
    std::string dir;
    while (std::getline(ss, dir, ':')) {
        if (dir.empty()) dir = ".";
        std::string cand = path_join(dir, prog);
        if (::access(cand.c_str(), X_OK) == 0) return cand;
    }
    return {};
}

// RAII temp directory.
class TempDir {
public:
    explicit TempDir(bool keep) : keep_(keep) {
        const char *t = std::getenv("TMPDIR");
        std::string tmpl = path_join((t && *t) ? t : "/tmp", "GeotiffToDmr.XXXXXX");
        std::vector<char> buf(tmpl.begin(), tmpl.end());
        buf.push_back('\0');
        if (!::mkdtemp(buf.data()))
            throw CustomError(kInternal, std::string("mkdtemp failed: ") + std::strerror(errno));
        path_ = buf.data();
    }
    ~TempDir() {
        if (keep_) {
            std::cerr << "GeotiffToDmr: kept temp files in " << path_ << '\n';
            return;
        }
        remove_tree(path_);
    }
    TempDir(const TempDir &) = delete;
    TempDir &operator=(const TempDir &) = delete;
    const std::string &path() const { return path_; }

private:
    std::string path_;
    bool keep_;
};

// ---------------------------------------------------------------------------
// Step 1: input validation
// ---------------------------------------------------------------------------
std::string validate_input(const std::string &input) {
    char resolved[PATH_MAX];
    if (!::realpath(input.c_str(), resolved)) {
        if (errno == ENOENT) throw CustomError(kBadInput, input + ": no such file");
        throw CustomError(kBadInput, input + ": " + std::strerror(errno));
    }
    std::string p = resolved;

    struct stat st;
    if (::stat(p.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
        throw CustomError(kBadInput, input + ": not a regular file");
    if (::access(p.c_str(), R_OK) != 0) throw CustomError(kBadInput, input + ": not readable");

    // The BES catalog excludes dot-files by default.
    if (path_basename(p).front() == '.')
        throw CustomError(kBadInput, input + ": hidden files (leading '.') are not served by the BES");

    // The generated TypeMatch maps .tif/.tiff (any case) to the gdal handler.
    std::string ext = path_extension_lower(p);
    if (ext != ".tif" && ext != ".tiff")
        throw CustomError(kBadInput, input + ": expected a .tif or .tiff file");

    // TIFF magic: "II*\0" / "MM\0*" (classic) or "II+\0" / "MM\0+" (BigTIFF).
    unsigned char m[4] = {0, 0, 0, 0};
    {
        std::ifstream in(p, std::ios::binary);
        in.read(reinterpret_cast<char *>(m), 4);
        if (in.gcount() != 4) throw CustomError(kBadInput, input + ": file too short to be a TIFF");
    }
    bool le = m[0] == 'I' && m[1] == 'I' && (m[2] == 42 || m[2] == 43) && m[3] == 0;
    bool be = m[0] == 'M' && m[1] == 'M' && m[2] == 0 && (m[3] == 42 || m[3] == 43);
    if (!le && !be) throw CustomError(kBadInput, input + ": not a TIFF file (bad magic bytes)");

#ifdef GEOTIFFTODMR_WITH_GDAL
    GDALAllRegister();
    CPLPushErrorHandler(CPLQuietErrorHandler);
    GDALDatasetH ds = GDALOpen(p.c_str(), GA_ReadOnly);
    CPLPopErrorHandler();
    if (!ds) throw CustomError(kBadInput, input + ": GDAL cannot open the file");
    std::string driver = GDALGetDriverShortName(GDALGetDatasetDriver(ds));
    double gt[6];
    bool has_gt = GDALGetGeoTransform(ds, gt) == CE_None;
    const char *wkt = GDALGetProjectionRef(ds);
    bool has_srs = wkt && *wkt;
    int bands = GDALGetRasterCount(ds);
    GDALClose(ds);
    if (driver != "GTiff")
        throw CustomError(kBadInput, input + ": GDAL driver is " + driver + ", expected GTiff");
    if (bands < 1) throw CustomError(kBadInput, input + ": no raster bands");
    if (!has_gt && !has_srs)
        throw CustomError(kBadInput, input + ": not georeferenced (no geotransform or CRS)");
#endif

    return p;
}

// ---------------------------------------------------------------------------
// Step 2: BES configuration and command file
// ---------------------------------------------------------------------------
std::string find_bes_lib_dir(const CmdArgs &args, const std::string &bes_exe) {
    const std::string gdal_lib = "libgdal_module.so";
    if (!args.bes_lib_dir.empty()) {
        if (!path_exists(path_join(args.bes_lib_dir, gdal_lib)))
            throw CustomError(kBadInput, "--bes-lib-dir " + args.bes_lib_dir + " has no " + gdal_lib);
        return args.bes_lib_dir;
    }

    std::vector<std::string> cands;
    if (const char *env = std::getenv("BES_LIB_DIR")) cands.emplace_back(env);
    if (*GEOTIFFTODMR_DEFAULT_BES_LIB_DIR) cands.emplace_back(GEOTIFFTODMR_DEFAULT_BES_LIB_DIR);
    if (!bes_exe.empty()) {
        std::string prefix = path_dirname(path_dirname(bes_exe));
        cands.push_back(prefix + "/lib64/bes");
        cands.push_back(prefix + "/lib/bes");
    }
    for (const char *d : {"/usr/lib64/bes", "/usr/lib/bes", "/usr/local/lib64/bes",
                          "/usr/local/lib/bes", "/opt/bes/lib/bes", "/opt/bes/lib64/bes"})
        cands.emplace_back(d);

    for (const auto &d : cands)
        if (path_exists(path_join(d, gdal_lib))) return d;
    return {};
}

std::string make_bes_conf(const CmdArgs &args, const std::string &tmp, const std::string &root,
                          const std::string &lib_dir) {
    std::ostringstream c;
    c << "# Generated by GeotiffToDmr " << kVersion << "\n";

    if (!args.bes_conf.empty()) {
        // Start from the site's own configuration, then override what we need.
        c << "BES.Include=" << path_absolute(args.bes_conf) << "\n";
    } else {
        if (lib_dir.empty())
            throw CustomError(kBadInput,
                            "cannot find the BES module directory (libgdal_module.so); "
                            "pass --bes-lib-dir or --bes-conf");
        c << "BES.ServerAdministrator=email:admin@localhost\n"
             "BES.ServerAdministrator+=organization:GeotiffToDmr\n"
             "BES.modules=dap,cmd,gdal\n"
          << "BES.module.dap=" << path_join(lib_dir, "libdap_module.so") << "\n"
          << "BES.module.cmd=" << path_join(lib_dir, "libdap_xml_module.so") << "\n"
          << "BES.module.gdal=" << path_join(lib_dir, "libgdal_module.so") << "\n"
          << "BES.Data.RootDirectory=/dev/null\n"
             "BES.Catalog.catalog.Include=;\n"
             "BES.Catalog.catalog.Exclude=^\\..*;\n"
             "BES.Info.Buffered=no\n"
             "BES.Info.Type=xml\n"
             "BES.Container.Persistence=strict\n"
             "BES.Memory.GlobalArea.EmergencyPoolSize=1\n"
             "BES.Memory.GlobalArea.MaximumHeapSize=20\n"
             "BES.Memory.GlobalArea.Verbose=no\n"
             "BES.Memory.GlobalArea.ControlHeap=no\n"
             "BES.ProcessManagerMethod=multiple\n"
             "BES.DefaultResponseMethod=POST\n"
          << "BES.UncompressCache.dir=" << path_join(tmp, "cache") << "\n"
          << "BES.UncompressCache.prefix=uncompress_cache\n"
             "BES.UncompressCache.size=500\n";
    }

    // Always overridden: per-run catalog root, handler mapping, log location.
    c << "BES.LogName=" << path_join(tmp, "bes.log") << "\n"
      << "BES.LogVerbose=no\n"
      << "BES.Catalog.catalog.RootDirectory=" << root << "\n"
      << "BES.Catalog.catalog.TypeMatch=gdal:.*\\.(tif|TIF|tiff|TIFF|Tif|Tiff)$;\n"
      << "BES.FollowSymLinks=Yes\n"
      << "BES.Catalog.catalog.FollowSymLinks=Yes\n";

    for (const auto &kv : args.extra_keys) c << kv << "\n";
    return c.str();
}

std::string make_bescmd(const std::string &file_in_root) {
    std::ostringstream x;
    x << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         "<bes:request xmlns:bes=\"http://xml.opendap.org/ns/bes/1.0#\" reqID=\"GeotiffToDmr\">\n"
         "  <bes:setContext name=\"dap_explicit_containers\">no</bes:setContext>\n"
         "  <bes:setContext name=\"errors\">xml</bes:setContext>\n"
         "  <bes:setContainer name=\"c\" space=\"catalog\">/"
      << xml_escape(file_in_root)
      << "</bes:setContainer>\n"
         "  <bes:define name=\"d\" space=\"default\">\n"
         "    <bes:container name=\"c\" />\n"
         "  </bes:define>\n"
         "  <bes:get type=\"dmr\" definition=\"d\" />\n"
         "</bes:request>\n";
    return x.str();
}

// ---------------------------------------------------------------------------
// Step 3: run besstandalone with a timeout
// ---------------------------------------------------------------------------
struct RunResult {
    int exit_status = -1;   // valid if !signaled
    bool signaled = false;
    int signal = 0;
    bool timed_out = false;
    std::string out;
    std::string err;
};

RunResult run_besstandalone(const std::string &exe, const std::string &conf,
                            const std::string &cmd, const std::string &tmp, int timeout_sec) {
    std::string out_path = path_join(tmp, "stdout.txt");
    std::string err_path = path_join(tmp, "stderr.txt");

    std::vector<std::string> args = {exe, "-c", conf, "-i", cmd};
    std::vector<char *> argv;
    for (auto &a : args) argv.push_back(&a[0]);
    argv.push_back(nullptr);

    log_verbose("running: " + exe + " -c " + conf + " -i " + cmd);

    pid_t pid = ::fork();
    if (pid < 0) throw CustomError(kInternal, std::string("fork failed: ") + std::strerror(errno));

    if (pid == 0) {
        // Child: own process group so a timeout kills any grandchildren too.
        ::setpgid(0, 0);
        int fo = ::open(out_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        int fe = ::open(err_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        int fi = ::open("/dev/null", O_RDONLY);
        if (fo < 0 || fe < 0 || fi < 0) ::_exit(126);
        ::dup2(fi, STDIN_FILENO);
        ::dup2(fo, STDOUT_FILENO);
        ::dup2(fe, STDERR_FILENO);
        ::close(fi);
        ::close(fo);
        ::close(fe);
        if (::chdir(tmp.c_str()) != 0) ::_exit(126);
        ::execvp(argv[0], argv.data());
        ::_exit(127);
    }

    ::setpgid(pid, pid);  // also from the parent, to avoid a race
    RunResult r;
    int status = 0;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout_sec);
    for (;;) {
        pid_t w = ::waitpid(pid, &status, WNOHANG);
        if (w == pid) break;
        if (w < 0 && errno != EINTR)
            throw CustomError(kInternal, std::string("waitpid failed: ") + std::strerror(errno));
        if (std::chrono::steady_clock::now() >= deadline) {
            r.timed_out = true;
            ::kill(-pid, SIGKILL);
            ::kill(pid, SIGKILL);
            while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {
            }
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    if (WIFEXITED(status)) {
        r.exit_status = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        r.signaled = true;
        r.signal = WTERMSIG(status);
    }
    r.out = read_file(out_path);
    r.err = read_file(err_path);
    return r;
}

// ---------------------------------------------------------------------------
// Step 4: interpret the response
// ---------------------------------------------------------------------------
std::string extract_tag(const std::string &xml, const std::string &tag) {
    // Text content of the first <Tag> or <ns:Tag> element.
    const std::regex re("<(?:[A-Za-z_][\\w.-]*:)?" + tag + "(?:\\s[^>]*)?>([\\s\\S]*?)</");
    std::smatch m;
    if (std::regex_search(xml, m, re)) return trim(m[1].str());
    return {};
}

std::string interpret_response(const RunResult &r, const std::string &input, int timeout_sec) {
    if (r.timed_out)
        throw CustomError(kTimeout, input + ": besstandalone timed out after " +
                                      std::to_string(timeout_sec) + " s");
    if (!r.signaled && r.exit_status == 127)
        throw CustomError(kInternal, "could not execute besstandalone (not installed or not on PATH?)");
    if (!r.signaled && r.exit_status == 126)
        throw CustomError(kInternal, "could not set up the besstandalone process");

    const std::string &out = r.out;

    // BES errors come back as XML (errors context = xml): <bes:BESError> or a DAP4 <Error>.
    bool is_error = out.find("BESError") != std::string::npos ||
                    out.find("<Error") != std::string::npos ||
                    out.find(":Error") != std::string::npos;

    size_t ds = out.find("<Dataset");
    bool is_dmr = ds != std::string::npos &&
                  out.find("http://xml.opendap.org/ns/DAP/4.0#") != std::string::npos;

    if (is_error || !is_dmr) {
        std::string msg = extract_tag(out, "Message");
        if (msg.empty()) {
            std::string err = trim(r.err);
            if (err.size() > 2000) err = err.substr(0, 2000) + " ...";
            std::string head = trim(out);
            if (head.size() > 2000) head = head.substr(0, 2000) + " ...";
            msg = !err.empty() ? err : (!head.empty() ? head : "no output from besstandalone");
        }
        std::ostringstream os;
        os << input << ": BES did not return a DMR";
        if (r.signaled) os << " (besstandalone killed by signal " << r.signal << ")";
        else if (r.exit_status != 0) os << " (exit status " << r.exit_status << ")";
        os << ": " << msg;
        throw CustomError(kBesError, os.str());
    }

    if (r.signaled || r.exit_status != 0)
        log_verbose("warning: besstandalone exited with status " +
                    std::to_string(r.exit_status) + " but returned a DMR");

    // Keep from the XML declaration (if it directly precedes the Dataset) onward.
    size_t start = out.rfind("<?xml", ds);
    if (start == std::string::npos) start = ds;
    std::string dmr = out.substr(start);
    size_t end = dmr.rfind("</Dataset>");
    if (end == std::string::npos)
        throw CustomError(kBesError, input + ": DMR response is truncated (no </Dataset>)");
    dmr.resize(end + std::string("</Dataset>").size());
    dmr += '\n';
    return dmr;
}

// ---------------------------------------------------------------------------
// Per-file driver
// ---------------------------------------------------------------------------
std::string output_path_for(const CmdArgs &args, const std::string &input) {
    if (!args.output.empty()) return args.output;
    std::string name = path_basename(input) + ".dmr";  // Hyrax style: foargs.tif.dmr
    if (!args.out_dir.empty()) return path_join(args.out_dir, name);
    return path_join(path_dirname(input), name);
}

void write_output(const std::string &dest, const std::string &dmr) {
    if (dest == "-") {
        std::cout << dmr;
        std::cout.flush();
        return;
    }
    if (dest.find('/') != std::string::npos) make_dirs(path_dirname(dest));

    // Atomic replace: write next to the destination, then rename.
    std::string tmp = dest + ".tmp." + std::to_string(::getpid());
    write_file(tmp, dmr);
    if (::rename(tmp.c_str(), dest.c_str()) != 0) {
        int e = errno;
        ::remove(tmp.c_str());
        throw CustomError(kInternal, "cannot write " + dest + ": " + std::strerror(e));
    }
}

void process_one(const CmdArgs &args, const std::string &input, const std::string &bes_exe,
                 const std::string &lib_dir) {
    std::string in = validate_input(input);
    log_verbose("input: " + in);

    TempDir tmp(args.keep_temp);
    make_dirs(path_join(tmp.path(), "cache"));

    std::string conf = path_join(tmp.path(), "bes.conf");
    std::string cmd = path_join(tmp.path(), "get_dmr.bescmd");
    write_file(conf, make_bes_conf(args, tmp.path(), path_dirname(in), lib_dir));
    write_file(cmd, make_bescmd(path_basename(in)));

    RunResult r = run_besstandalone(bes_exe, conf, cmd, tmp.path(), args.timeout_sec);
    std::string dmr = interpret_response(r, input, args.timeout_sec);

    std::string dest = output_path_for(args, in);
    write_output(dest, dmr);
    if (dest != "-") log_verbose("wrote " + dest);
}

}  // namespace

int main(int argc, char **argv) {
    CmdArgs args;
    try {
        args = parse_args(argc, argv);
    } catch (const CustomError &e) {
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
    } catch (const CustomError &e) {
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
        } catch (const CustomError &e) {
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
