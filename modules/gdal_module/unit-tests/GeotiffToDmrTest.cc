// GeotiffToDmrTest.cc
//
// CppUnit tests for the functions in ../GeotiffToDmr.cc. Each test runs in
// its own scratch directory under the build directory; besstandalone is
// replaced by small shell scripts written there, so these tests need no BES.
// (The autotest suite in ../tests runs the tool against the real BES.)
//
// Run all tests:   ./GeotiffToDmrTest
// List tests:      ./GeotiffToDmrTest -h
// Run some:        ./GeotiffToDmrTest test_trim test_which_finds_program_on_path

#include "config.h"

#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

#include "run_tests_cppunit.h"
#include "test_config.h"

#include "GeotiffToDmr.h"

using namespace std;

namespace geotiff_to_dmr {

// A valid 1x1 8-bit TIFF with no geotransform and no CRS.
static const unsigned char kPlainTiff[] = {
    0x49, 0x49, 0x2a, 0x00, 0x08, 0x00, 0x00, 0x00, 0x09, 0x00,
    0x00, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x02, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x03, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x11, 0x01, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7a, 0x00, 0x00, 0x00,
    0x15, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x16, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x17, 0x01, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x07};

static const char *const kDmr =
    "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n"
    "<Dataset xmlns=\"http://xml.opendap.org/ns/DAP/4.0#\" dapVersion=\"4.0\" dmrVersion=\"1.0\" name=\"cea.tif\">\n"
    "    <Dimension name=\"northing\" size=\"515\"/>\n"
    "</Dataset>";

// Environment variables the code reads; saved in setUp, restored in tearDown.
static const char *const kEnvVars[] = {"PATH", "TMPDIR", "BESSTANDALONE", "BES_LIB_DIR"};

class GeotiffToDmrTest : public CppUnit::TestFixture {
private:
    string d_work;       // per-test scratch directory (canonical path)
    string d_data;       // modules/gdal_module/data
    char d_cwd[PATH_MAX];
    vector<pair<bool, string>> d_env;

    // ---- helpers ---------------------------------------------------------

    // Require that f() throws GeoToDMRInternalError with this code and a
    // message containing 'needle'.
    static void assert_error(const function<void()> &f, ExitCode code, const string &needle) {
        try {
            f();
        }
        catch (const GeoToDMRInternalError &e) {
            DBG(cerr << "Caught expected error: " << e.what() << endl);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(string("exit code for: ") + e.what(), static_cast<int>(code),
                                         static_cast<int>(e.code));
            CPPUNIT_ASSERT_MESSAGE(string("'") + needle + "' not in: " + e.what(),
                                   string(e.what()).find(needle) != string::npos);
            return;
        }
        CPPUNIT_FAIL("expected a GeoToDMRInternalError containing: " + needle);
    }

    static bool contains(const string &haystack, const string &needle) {
        return haystack.find(needle) != string::npos;
    }

    static CmdArgs parse(const vector<string> &words) {
        vector<string> storage = {"GeotiffToDmr"};
        storage.insert(storage.end(), words.begin(), words.end());
        vector<char *> argv;
        for (auto &w : storage) argv.push_back(&w[0]);
        argv.push_back(nullptr);
        return parse_args(static_cast<int>(storage.size()), argv.data());
    }

    string in_work(const string &name) const { return d_work + "/" + name; }

    string copy_data(const string &from, const string &to) const {
        string dest = in_work(to);
        write_file(dest, read_file(d_data + "/" + from));
        return dest;
    }

    string write_bytes(const string &name, const string &bytes) const {
        string p = in_work(name);
        write_file(p, bytes);
        return p;
    }

    // Write an executable /bin/sh script standing in for besstandalone.
    string write_script(const string &name, const string &body) const {
        string p = in_work(name);
        write_file(p, "#!/bin/sh\n" + body + "\n");
        CPPUNIT_ASSERT(::chmod(p.c_str(), 0755) == 0);
        return p;
    }

    // Capture what f() writes to the given stream.
    static string capture(ostream &os, const function<void()> &f) {
        ostringstream buf;
        streambuf *old = os.rdbuf(buf.rdbuf());
        try {
            f();
        }
        catch (...) {
            os.rdbuf(old);
            throw;
        }
        os.rdbuf(old);
        return buf.str();
    }

    static string realpath_of(const string &p) {
        char r[PATH_MAX];
        CPPUNIT_ASSERT_MESSAGE("realpath " + p, ::realpath(p.c_str(), r) != nullptr);
        return r;
    }

    static bool is_dir(const string &p) {
        struct stat st;
        return ::stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
    }

public:
    void setUp() override {
        d_env.clear();
        for (const char *name : kEnvVars) {
            const char *v = getenv(name);
            d_env.emplace_back(v != nullptr, v ? v : "");
        }
        unsetenv("BESSTANDALONE");
        unsetenv("BES_LIB_DIR");

        CPPUNIT_ASSERT(::getcwd(d_cwd, sizeof d_cwd) != nullptr);
        string tmpl = string(TEST_BUILD_DIR) + "/GeotiffToDmrTest.XXXXXX";
        vector<char> buf(tmpl.begin(), tmpl.end());
        buf.push_back('\0');
        CPPUNIT_ASSERT(::mkdtemp(buf.data()) != nullptr);
        d_work = realpath_of(buf.data());
        d_data = realpath_of(string(TEST_SRC_DIR) + "/../data");

        // Temp directories made by the code under test land in the scratch dir.
        setenv("TMPDIR", d_work.c_str(), 1);
        g_verbose = false;
    }

    void tearDown() override {
        CPPUNIT_ASSERT(::chdir(d_cwd) == 0);
        for (size_t i = 0; i < d_env.size(); ++i) {
            if (d_env[i].first) setenv(kEnvVars[i], d_env[i].second.c_str(), 1);
            else unsetenv(kEnvVars[i]);
        }
        g_verbose = false;
        // Undo test_validate_input_unreadable's chmod so the tree can go.
        ::chmod(in_work("cea.tif").c_str(), 0644);
        if (!debug) remove_tree(d_work);
    }

    // ---- path and string helpers ------------------------------------------

    void test_path_join() {
        CPPUNIT_ASSERT_EQUAL(string("a/b"), path_join("a", "b"));
        CPPUNIT_ASSERT_EQUAL(string("a/b"), path_join("a/", "b"));
        CPPUNIT_ASSERT_EQUAL(string("/b"), path_join("/", "b"));
        CPPUNIT_ASSERT_EQUAL(string("b"), path_join("", "b"));
    }

    void test_path_dirname() {
        CPPUNIT_ASSERT_EQUAL(string("/a"), path_dirname("/a/b.tif"));
        CPPUNIT_ASSERT_EQUAL(string("a/b"), path_dirname("a/b/c"));
        CPPUNIT_ASSERT_EQUAL(string("/"), path_dirname("/b.tif"));
        CPPUNIT_ASSERT_EQUAL(string("."), path_dirname("b.tif"));
    }

    void test_path_basename() {
        CPPUNIT_ASSERT_EQUAL(string("b.tif"), path_basename("/a/b.tif"));
        CPPUNIT_ASSERT_EQUAL(string("b.tif"), path_basename("b.tif"));
        CPPUNIT_ASSERT_EQUAL(string(""), path_basename("/a/"));
    }

    void test_path_extension_lower() {
        CPPUNIT_ASSERT_EQUAL(string(".tif"), path_extension_lower("/a/B.TIF"));
        CPPUNIT_ASSERT_EQUAL(string(".tiff"), path_extension_lower("x.TiFf"));
        CPPUNIT_ASSERT_EQUAL(string(".tif"), path_extension_lower("x.tar.tif"));
        CPPUNIT_ASSERT_EQUAL(string(""), path_extension_lower("noext"));
        CPPUNIT_ASSERT_EQUAL(string(""), path_extension_lower(".hidden"));
        CPPUNIT_ASSERT_EQUAL(string(""), path_extension_lower("dir.d/file"));
    }

    void test_path_exists() {
        CPPUNIT_ASSERT(path_exists(d_work));
        CPPUNIT_ASSERT(!path_exists(in_work("nosuch")));
    }

    void test_path_absolute() {
        CPPUNIT_ASSERT_EQUAL(string("/x/y"), path_absolute("/x/y"));
        CPPUNIT_ASSERT(::chdir(d_work.c_str()) == 0);
        CPPUNIT_ASSERT_EQUAL(in_work("rel/file"), path_absolute("rel/file"));
    }

    void test_make_dirs_and_remove_tree() {
        string deep = in_work("a/b/c");
        make_dirs(deep);
        CPPUNIT_ASSERT(is_dir(deep));
        make_dirs(deep);  // existing: no error
        write_file(deep + "/f.txt", "x");
        remove_tree(in_work("a"));
        CPPUNIT_ASSERT(!path_exists(in_work("a")));
    }

    void test_make_dirs_error() {
        write_file(in_work("plain"), "x");
        assert_error([this] { make_dirs(in_work("plain/sub")); }, kInternal, "cannot create");
    }

    void test_read_write_file() {
        string p = in_work("f.bin");
        string content("a\0b\n", 4);
        write_file(p, content);
        CPPUNIT_ASSERT_EQUAL(content, read_file(p));
        write_file(p, "short");  // truncates
        CPPUNIT_ASSERT_EQUAL(string("short"), read_file(p));
        CPPUNIT_ASSERT_EQUAL(string(""), read_file(in_work("nosuch")));
    }

    void test_write_file_error() {
        assert_error([this] { write_file(in_work("nosuch/f"), "x"); }, kInternal, "cannot write");
    }

    void test_xml_escape() {
        CPPUNIT_ASSERT_EQUAL(string("a&amp;b&lt;c&gt;d&quot;e&apos;f"), xml_escape("a&b<c>d\"e'f"));
        CPPUNIT_ASSERT_EQUAL(string("plain.tif"), xml_escape("plain.tif"));
    }

    void test_trim() {
        CPPUNIT_ASSERT_EQUAL(string("a b"), trim(" \t\r\na b \n"));
        CPPUNIT_ASSERT_EQUAL(string(""), trim(" \n\t "));
        CPPUNIT_ASSERT_EQUAL(string("x"), trim("x"));
    }

    // ---- which -------------------------------------------------------------

    void test_which_absolute_path() {
        string exe = write_script("tool", "true");
        CPPUNIT_ASSERT_EQUAL(exe, which(exe));
    }

    void test_which_relative_path_made_absolute() {
        string exe = write_script("tool", "true");
        CPPUNIT_ASSERT(::chdir(d_work.c_str()) == 0);
        // Absolute (the child chdir()s before exec) and the same file; the
        // path is not normalised, e.g. <dir>/./tool.
        string found = which("./tool");
        CPPUNIT_ASSERT_MESSAGE(found, !found.empty() && found.at(0) == '/');
        CPPUNIT_ASSERT_EQUAL(exe, realpath_of(found));
    }

    void test_which_finds_program_on_path() {
        make_dirs(in_work("bin"));
        write_script("bin/fake-bes", "true");
        setenv("PATH", (in_work("nosuchdir") + "::" + in_work("bin")).c_str(), 1);
        CPPUNIT_ASSERT_EQUAL(in_work("bin/fake-bes"), which("fake-bes"));
    }

    void test_which_not_found() {
        setenv("PATH", d_work.c_str(), 1);
        CPPUNIT_ASSERT_EQUAL(string(""), which("no-such-program"));
        CPPUNIT_ASSERT_EQUAL(string(""), which(in_work("no-such-program")));
        write_file(in_work("not-exec"), "x");
        CPPUNIT_ASSERT_EQUAL(string(""), which(in_work("not-exec")));
        CPPUNIT_ASSERT_EQUAL(string(""), which("not-exec"));
    }

    // ---- logging and usage ----------------------------------------------------

    void test_log_verbose() {
        g_verbose = false;
        CPPUNIT_ASSERT_EQUAL(string(""), capture(cerr, [] { log_verbose("hello"); }));
        g_verbose = true;
        CPPUNIT_ASSERT_EQUAL(string("GeotiffToDmr: hello\n"), capture(cerr, [] { log_verbose("hello"); }));
    }

    void test_print_usage() {
        ostringstream os;
        print_usage(os);
        CPPUNIT_ASSERT(contains(os.str(), "Usage: GeotiffToDmr [options] INPUT.tif"));
        CPPUNIT_ASSERT(contains(os.str(), "Exit codes: 0 ok, 1 bad input/usage, 2 BES error, 3 timeout, 4 internal."));
    }

    // ---- parse_args (-h/--help and --version call exit(); tests/ covers them) --

    void test_parse_args_defaults() {
        CmdArgs args = parse({"a.tif"});
        CPPUNIT_ASSERT_EQUAL(size_t(1), args.inputs.size());
        CPPUNIT_ASSERT_EQUAL(string("a.tif"), args.inputs.at(0));
        CPPUNIT_ASSERT_EQUAL(60, args.timeout_sec);
        CPPUNIT_ASSERT(!args.keep_temp);
        CPPUNIT_ASSERT(!args.verbose);
        CPPUNIT_ASSERT(args.output.empty() && args.out_dir.empty());
        CPPUNIT_ASSERT(args.bes_lib_dir.empty() && args.bes_conf.empty() && args.extra_keys.empty());
        CPPUNIT_ASSERT(!args.besstandalone.empty());  // the compiled-in default
    }

    void test_parse_args_all_options() {
        CmdArgs args = parse({"-o", "out.dmr", "--besstandalone", "/b/bes", "--bes-lib-dir", "/l",
                              "--bes-conf", "c.conf", "--key", "A=1", "--key", "B=2=3", "--timeout", "7",
                              "--keep-temp", "-v", "in.tif"});
        CPPUNIT_ASSERT_EQUAL(string("out.dmr"), args.output);
        CPPUNIT_ASSERT_EQUAL(string("/b/bes"), args.besstandalone);
        CPPUNIT_ASSERT_EQUAL(string("/l"), args.bes_lib_dir);
        CPPUNIT_ASSERT_EQUAL(string("c.conf"), args.bes_conf);
        CPPUNIT_ASSERT_EQUAL(size_t(2), args.extra_keys.size());
        CPPUNIT_ASSERT_EQUAL(string("A=1"), args.extra_keys.at(0));
        CPPUNIT_ASSERT_EQUAL(string("B=2=3"), args.extra_keys.at(1));
        CPPUNIT_ASSERT_EQUAL(7, args.timeout_sec);
        CPPUNIT_ASSERT(args.keep_temp);
        CPPUNIT_ASSERT(args.verbose);
        CPPUNIT_ASSERT_EQUAL(string("in.tif"), args.inputs.at(0));
    }

    void test_parse_args_long_and_short_forms() {
        CmdArgs args = parse({"--output", "x.dmr", "--verbose", "a.tif"});
        CPPUNIT_ASSERT_EQUAL(string("x.dmr"), args.output);
        CPPUNIT_ASSERT(args.verbose);
        args = parse({"-d", "dir", "a.tif", "b.tif"});
        CPPUNIT_ASSERT_EQUAL(string("dir"), args.out_dir);
        args = parse({"--out-dir", "dir2", "a.tif"});
        CPPUNIT_ASSERT_EQUAL(string("dir2"), args.out_dir);
    }

    void test_parse_args_double_dash_and_inputs() {
        CmdArgs args = parse({"a.tif", "--", "-odd.tif", "--keep-temp"});
        CPPUNIT_ASSERT_EQUAL(size_t(3), args.inputs.size());
        CPPUNIT_ASSERT_EQUAL(string("-odd.tif"), args.inputs.at(1));
        CPPUNIT_ASSERT_EQUAL(string("--keep-temp"), args.inputs.at(2));
        CPPUNIT_ASSERT(!args.keep_temp);
        args = parse({"-"});  // a lone '-' is an input name, not an option
        CPPUNIT_ASSERT_EQUAL(string("-"), args.inputs.at(0));
    }

    void test_parse_args_besstandalone_from_environment() {
        setenv("BESSTANDALONE", "/env/besstandalone", 1);
        CPPUNIT_ASSERT_EQUAL(string("/env/besstandalone"), parse({"a.tif"}).besstandalone);
        CPPUNIT_ASSERT_EQUAL(string("/opt/bes"), parse({"--besstandalone", "/opt/bes", "a.tif"}).besstandalone);
        setenv("BESSTANDALONE", "", 1);  // empty: ignored
        CPPUNIT_ASSERT(parse({"a.tif"}).besstandalone != "");
    }

    void test_parse_args_errors() {
        assert_error([] { parse({}); }, kBadInput, "no input file given");
        assert_error([] { parse({"--bogus", "a.tif"}); }, kBadInput, "unknown option: --bogus");
        assert_error([] { parse({"a.tif", "-o"}); }, kBadInput, "missing value for -o");
        assert_error([] { parse({"a.tif", "--key"}); }, kBadInput, "missing value for --key");
        assert_error([] { parse({"-o", "x", "a.tif", "b.tif"}); }, kBadInput, "-o/--output works with a single input");
        assert_error([] { parse({"-o", "x", "-d", "d", "a.tif"}); }, kBadInput, "use either -o/--output or --out-dir");
        assert_error([] { parse({"--timeout", "0", "a.tif"}); }, kBadInput, "--timeout must be > 0");
        assert_error([] { parse({"--timeout", "-5", "a.tif"}); }, kBadInput, "--timeout must be > 0");
        assert_error([] { parse({"--timeout", "abc", "a.tif"}); }, kBadInput, "invalid --timeout: abc");
        assert_error([] { parse({"--key", "NOEQUALS", "a.tif"}); }, kBadInput, "--key expects KEY=VALUE");
    }

    // ---- TempDir ---------------------------------------------------------------

    void test_temp_dir_created_and_removed() {
        string path;
        {
            TempDir t(false);
            path = t.path();
            CPPUNIT_ASSERT(is_dir(path));
            CPPUNIT_ASSERT_EQUAL(d_work, path_dirname(path));  // honours $TMPDIR
            CPPUNIT_ASSERT(contains(path_basename(path), "GeotiffToDmr."));
            make_dirs(path + "/cache/sub");
            write_file(path + "/cache/sub/f", "x");
        }
        CPPUNIT_ASSERT(!path_exists(path));
    }

    void test_temp_dir_keep() {
        string path;
        string err = capture(cerr, [&path] {
            TempDir t(true);
            path = t.path();
        });
        CPPUNIT_ASSERT(is_dir(path));
        CPPUNIT_ASSERT_EQUAL("GeotiffToDmr: kept temp files in " + path + "\n", err);
    }

    void test_temp_dir_unique() {
        TempDir a(false), b(false);
        CPPUNIT_ASSERT(a.path() != b.path());
    }

    void test_temp_dir_error() {
        setenv("TMPDIR", in_work("nosuch").c_str(), 1);
        assert_error([] { TempDir t(false); }, kInternal, "mkdtemp failed");
    }

    // ---- validate_input ----------------------------------------------------------

    void test_validate_input_geotiff() {
        string p = copy_data("cea.tif", "cea.tif");
        CPPUNIT_ASSERT_EQUAL(p, validate_input(p));
    }

    void test_validate_input_returns_canonical_path() {
        copy_data("cea.tif", "cea.tif");
        make_dirs(in_work("sub"));
        CPPUNIT_ASSERT(::chdir(in_work("sub").c_str()) == 0);
        CPPUNIT_ASSERT_EQUAL(in_work("cea.tif"), validate_input("../cea.tif"));
        CPPUNIT_ASSERT(::symlink(in_work("cea.tif").c_str(), in_work("link.tif").c_str()) == 0);
        CPPUNIT_ASSERT_EQUAL(in_work("cea.tif"), validate_input(in_work("link.tif")));
    }

    void test_validate_input_extension_any_case() {
        string p = copy_data("cea.tif", "CEA.TiFf");
        CPPUNIT_ASSERT_EQUAL(p, validate_input(p));
    }

    void test_validate_input_errors() {
        assert_error([this] { validate_input(in_work("nosuch.tif")); }, kBadInput, "no such file");
        make_dirs(in_work("dir.tif"));
        assert_error([this] { validate_input(in_work("dir.tif")); }, kBadInput, "not a regular file");
        string png = copy_data("cea.tif", "cea.png");
        assert_error([png] { validate_input(png); }, kBadInput, "expected a .tif or .tiff file");
        string hidden = copy_data("cea.tif", ".hidden.tif");
        assert_error([hidden] { validate_input(hidden); }, kBadInput, "hidden files");
        string shorty = write_bytes("short.tif", "II*");
        assert_error([shorty] { validate_input(shorty); }, kBadInput, "file too short to be a TIFF");
        string text = write_bytes("text.tif", "hello world");
        assert_error([text] { validate_input(text); }, kBadInput, "not a TIFF file (bad magic bytes)");
    }

    // All four TIFF signatures pass the magic check; these files then fail in GDAL.
    void test_validate_input_tiff_magic_variants() {
        const string sigs[] = {string("II*\0", 4), string("MM\0*", 4), string("II+\0", 4), string("MM\0+", 4)};
        for (const string &sig : sigs) {
            string p = write_bytes("magic.tif", sig + string(64, '\0'));
            assert_error([p] { validate_input(p); }, kBadInput, "GDAL cannot open the file");
        }
    }

    void test_validate_input_not_georeferenced() {
        string p = write_bytes("plain.tif", string(reinterpret_cast<const char *>(kPlainTiff), sizeof kPlainTiff));
        assert_error([p] { validate_input(p); }, kBadInput, "not georeferenced");
    }

    void test_validate_input_unreadable() {
        if (::geteuid() == 0) {
            cerr << " [skipped test_validate_input_unreadable: running as root] ";
            return;
        }
        string p = copy_data("cea.tif", "cea.tif");
        CPPUNIT_ASSERT(::chmod(p.c_str(), 0) == 0);
        assert_error([p] { validate_input(p); }, kBadInput, "not readable");
    }

    // ---- find_bes_lib_dir ----------------------------------------------------------

    void test_find_bes_lib_dir_explicit() {
        make_dirs(in_work("lib"));
        write_file(in_work("lib/libgdal_module.so"), "");
        CmdArgs args;
        args.bes_lib_dir = in_work("lib");
        CPPUNIT_ASSERT_EQUAL(in_work("lib"), find_bes_lib_dir(args, ""));
        // A relative directory comes back absolute: it is written into bes.conf
        // and besstandalone runs in the temp directory.
        CPPUNIT_ASSERT(::chdir(d_work.c_str()) == 0);
        args.bes_lib_dir = "lib";
        CPPUNIT_ASSERT_EQUAL(in_work("lib"), find_bes_lib_dir(args, ""));
    }

    void test_find_bes_lib_dir_explicit_missing_module() {
        CmdArgs args;
        args.bes_lib_dir = d_work;
        assert_error([&args] { find_bes_lib_dir(args, ""); }, kBadInput, "has no libgdal_module.so");
    }

    void test_find_bes_lib_dir_from_environment() {
        make_dirs(in_work("envlib"));
        write_file(in_work("envlib/libgdal_module.so"), "");
        setenv("BES_LIB_DIR", in_work("envlib").c_str(), 1);
        CPPUNIT_ASSERT_EQUAL(in_work("envlib"), find_bes_lib_dir(CmdArgs(), ""));
    }

    // The tests below rely on GEOTIFFTODMR_DEFAULT_BES_LIB_DIR being the
    // same value the tool was built with ($(libdir)/bes); empty means the
    // definition is missing from this directory's Makefile.am.
    void test_compiled_default_lib_dir_is_set() {
        string dir = GEOTIFFTODMR_DEFAULT_BES_LIB_DIR;
        CPPUNIT_ASSERT_MESSAGE("GEOTIFFTODMR_DEFAULT_BES_LIB_DIR is empty", !dir.empty());
        CPPUNIT_ASSERT_MESSAGE(dir, dir.size() >= 4 && dir.compare(dir.size() - 4, 4, "/bes") == 0);
    }

    void test_find_bes_lib_dir_next_to_besstandalone() {
        // The compiled-in default is searched before <prefix>/lib/bes; if a BES
        // is installed there, this lookup legitimately finds that one.
        if (path_exists(string(GEOTIFFTODMR_DEFAULT_BES_LIB_DIR) + "/libgdal_module.so")) {
            cerr << " [skipped test_find_bes_lib_dir_next_to_besstandalone: BES installed] ";
            return;
        }
        make_dirs(in_work("prefix/bin"));
        make_dirs(in_work("prefix/lib/bes"));
        write_file(in_work("prefix/lib/bes/libgdal_module.so"), "");
        CPPUNIT_ASSERT_EQUAL(in_work("prefix/lib/bes"), find_bes_lib_dir(CmdArgs(), in_work("prefix/bin/besstandalone")));
        make_dirs(in_work("prefix/lib64/bes"));  // lib64 wins over lib
        write_file(in_work("prefix/lib64/bes/libgdal_module.so"), "");
        CPPUNIT_ASSERT_EQUAL(in_work("prefix/lib64/bes"), find_bes_lib_dir(CmdArgs(), in_work("prefix/bin/besstandalone")));
    }

    // ---- make_bes_conf and make_bescmd ----------------------------------------------

    void test_make_bes_conf_with_lib_dir() {
        CmdArgs args;
        string conf = make_bes_conf(args, "/t", "/data/dir", "/lib/bes");
        CPPUNIT_ASSERT(contains(conf, "BES.modules=dap,cmd,gdal\n"));
        CPPUNIT_ASSERT(contains(conf, "BES.module.dap=/lib/bes/libdap_module.so\n"));
        CPPUNIT_ASSERT(contains(conf, "BES.module.cmd=/lib/bes/libdap_xml_module.so\n"));
        CPPUNIT_ASSERT(contains(conf, "BES.module.gdal=/lib/bes/libgdal_module.so\n"));
        CPPUNIT_ASSERT(contains(conf, "BES.Catalog.catalog.RootDirectory=/data/dir\n"));
        CPPUNIT_ASSERT(contains(conf, "BES.LogName=/t/bes.log\n"));
        CPPUNIT_ASSERT(contains(conf, "BES.UncompressCache.dir=/t/cache\n"));
        CPPUNIT_ASSERT(!contains(conf, "BES.Include"));
    }

    void test_make_bes_conf_with_base_conf() {
        CPPUNIT_ASSERT(::chdir(d_work.c_str()) == 0);
        CmdArgs args;
        args.bes_conf = "site.conf";  // relative: written as an absolute path
        string conf = make_bes_conf(args, "/t", "/data/dir", "");
        CPPUNIT_ASSERT(contains(conf, "BES.Include=" + in_work("site.conf") + "\n"));
        CPPUNIT_ASSERT(!contains(conf, "BES.modules="));
        // The per-run settings are still written, after the Include.
        CPPUNIT_ASSERT(conf.find("BES.Include=") < conf.find("BES.Catalog.catalog.RootDirectory=/data/dir"));
        CPPUNIT_ASSERT(conf.find("BES.Include=") < conf.find("BES.Catalog.catalog.TypeMatch=gdal:"));
    }

    void test_make_bes_conf_extra_keys_come_last() {
        CmdArgs args;
        args.extra_keys = {"BES.Catalog.catalog.TypeMatch=nc:.*\\.nc$;", "X.Y=z"};
        string conf = make_bes_conf(args, "/t", "/r", "/lib");
        size_t ours = conf.find("BES.Catalog.catalog.TypeMatch=gdal:");
        size_t theirs = conf.find("BES.Catalog.catalog.TypeMatch=nc:");
        CPPUNIT_ASSERT(ours != string::npos && theirs != string::npos);
        CPPUNIT_ASSERT(ours < theirs);  // a later key replaces an earlier one in the BES
        CPPUNIT_ASSERT(theirs < conf.find("X.Y=z\n"));
    }

    void test_make_bes_conf_needs_lib_dir_or_conf() {
        assert_error([] { make_bes_conf(CmdArgs(), "/t", "/r", ""); }, kBadInput,
                     "cannot find the BES module directory");
    }

    // The TypeMatch regular expression must send every name validate_input()
    // accepts, in any capitalisation, to the gdal handler.
    void test_make_bes_conf_type_match() {
        string conf = make_bes_conf(CmdArgs(), "/t", "/r", "/lib");
        smatch m;
        CPPUNIT_ASSERT(regex_search(conf, m, regex("BES\\.Catalog\\.catalog\\.TypeMatch=gdal:(.*);\n")));
        regex type_match(m[1].str());
        for (const char *yes : {"a.tif", "a.TIF", "a.tiff", "a.TIFF", "a.Tif", "a.tIf", "a.tIFf", "dir/b.c.tif"})
            CPPUNIT_ASSERT_MESSAGE(yes, regex_match(string(yes), type_match));
        for (const char *no : {"a.tif.aux.xml", "a.png", "a.tifff", "atif", "a.ti"})
            CPPUNIT_ASSERT_MESSAGE(no, !regex_match(string(no), type_match));
    }

    void test_make_bescmd() {
        string cmd = make_bescmd("cea.tif");
        CPPUNIT_ASSERT(contains(cmd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));
        CPPUNIT_ASSERT(contains(cmd, "<bes:setContainer name=\"c\" space=\"catalog\">/cea.tif</bes:setContainer>"));
        CPPUNIT_ASSERT(contains(cmd, "<bes:container name=\"c\" />"));
        CPPUNIT_ASSERT(contains(cmd, "<bes:get type=\"dmr\" definition=\"d\" />"));
        CPPUNIT_ASSERT(contains(cmd, "<bes:setContext name=\"errors\">xml</bes:setContext>"));
        CPPUNIT_ASSERT(contains(cmd, "</bes:request>"));
    }

    void test_make_bescmd_escapes_file_name() {
        string cmd = make_bescmd("a&b<c>.tif");
        CPPUNIT_ASSERT(contains(cmd, ">/a&amp;b&lt;c&gt;.tif</bes:setContainer>"));
        CPPUNIT_ASSERT(!contains(cmd, "a&b"));
    }

    // ---- run_besstandalone -------------------------------------------------------

    void test_run_captures_output_and_status() {
        string exe = write_script("fake-bes", "echo out-text; echo err-text >&2; exit 3");
        TempDir t(false);
        RunResult r = run_besstandalone(exe, "c.conf", "c.bescmd", t.path(), 10);
        CPPUNIT_ASSERT_EQUAL(string("out-text\n"), r.out);
        CPPUNIT_ASSERT_EQUAL(string("err-text\n"), r.err);
        CPPUNIT_ASSERT_EQUAL(3, r.exit_status);
        CPPUNIT_ASSERT(!r.signaled && !r.timed_out);
    }

    void test_run_passes_arguments_and_directory() {
        string exe = write_script("fake-bes", "echo \"$# $1 $2 $3 $4\"; pwd -P");
        TempDir t(false);
        RunResult r = run_besstandalone(exe, "/x/bes.conf", "/x/get.bescmd", t.path(), 10);
        CPPUNIT_ASSERT_EQUAL("4 -c /x/bes.conf -i /x/get.bescmd\n" + realpath_of(t.path()) + "\n", r.out);
        CPPUNIT_ASSERT_EQUAL(0, r.exit_status);
    }

    void test_run_stdin_is_empty() {
        // 'cat' would wait for ever if stdin were the terminal or a pipe left open.
        string exe = write_script("fake-bes", "cat; echo done");
        TempDir t(false);
        RunResult r = run_besstandalone(exe, "c", "i", t.path(), 5);
        CPPUNIT_ASSERT(!r.timed_out);
        CPPUNIT_ASSERT_EQUAL(string("done\n"), r.out);
    }

    void test_run_timeout_kills_process_group() {
        // The child starts a grandchild that would write a file after 2 s.
        string marker = in_work("grandchild-ran");
        string exe = write_script("fake-bes", "(sleep 2; touch " + marker + ") & sleep 30");
        TempDir t(false);
        auto start = chrono::steady_clock::now();
        RunResult r = run_besstandalone(exe, "c", "i", t.path(), 1);
        double secs = chrono::duration<double>(chrono::steady_clock::now() - start).count();
        CPPUNIT_ASSERT(r.timed_out);
        CPPUNIT_ASSERT_MESSAGE("took " + to_string(secs) + " s", secs < 5);
        ::sleep(3);
        CPPUNIT_ASSERT_MESSAGE("grandchild survived the timeout", !path_exists(marker));
    }

    void test_run_killed_by_signal() {
        string exe = write_script("fake-bes", "kill -TERM $$; sleep 5");
        TempDir t(false);
        RunResult r = run_besstandalone(exe, "c", "i", t.path(), 10);
        CPPUNIT_ASSERT(r.signaled);
        CPPUNIT_ASSERT_EQUAL(15, r.signal);
        CPPUNIT_ASSERT(!r.timed_out);
    }

    void test_run_exec_failure() {
        TempDir t(false);
        RunResult r = run_besstandalone(in_work("no-such-program"), "c", "i", t.path(), 10);
        CPPUNIT_ASSERT_EQUAL(127, r.exit_status);
    }

    // ---- extract_tag and interpret_response -----------------------------------------

    void test_extract_tag() {
        CPPUNIT_ASSERT_EQUAL(string("oops"), extract_tag("<a><Message>oops</Message></a>", "Message"));
        CPPUNIT_ASSERT_EQUAL(string("ns oops"), extract_tag("<bes:Message>\n  ns oops\n</bes:Message>", "Message"));
        CPPUNIT_ASSERT_EQUAL(string("attr"), extract_tag("<Message lang=\"en\">attr</Message>", "Message"));
        CPPUNIT_ASSERT_EQUAL(string("first"), extract_tag("<Message>first</Message><Message>2</Message>", "Message"));
        CPPUNIT_ASSERT_EQUAL(string(""), extract_tag("<MessageX>no</MessageX>", "Message"));
        CPPUNIT_ASSERT_EQUAL(string(""), extract_tag("no xml here", "Message"));
    }

    static RunResult result(const string &out, int status = 0) {
        RunResult r;
        r.out = out;
        r.exit_status = status;
        return r;
    }

    void test_interpret_response_dmr() {
        string dmr = interpret_response(result(string(kDmr) + "\n"), "cea.tif", 60);
        CPPUNIT_ASSERT_EQUAL(string(kDmr) + "\n", dmr);
    }

    void test_interpret_response_strips_surrounding_text() {
        string dmr = interpret_response(result("noise before\n" + string(kDmr) + "\nnoise after\n"), "cea.tif", 60);
        CPPUNIT_ASSERT_EQUAL(string(kDmr) + "\n", dmr);
    }

    void test_interpret_response_without_xml_declaration() {
        string body = string(kDmr).substr(string(kDmr).find("<Dataset"));
        CPPUNIT_ASSERT_EQUAL(body + "\n", interpret_response(result("x\n" + body), "cea.tif", 60));
    }

    void test_interpret_response_dmr_despite_exit_status() {
        CPPUNIT_ASSERT_EQUAL(string(kDmr) + "\n", interpret_response(result(kDmr, 1), "cea.tif", 60));
    }

    void test_interpret_response_bes_error() {
        string err = "<?xml version=\"1.0\"?><response xmlns=\"http://xml.opendap.org/ns/bes/1.0#\">"
                     "<BESError><Type>3</Type><Message>Could not open the file</Message></BESError></response>";
        assert_error([err] { interpret_response(result(err), "cea.tif", 60); }, kBesError,
                     "cea.tif: BES did not return a DMR: Could not open the file");
    }

    void test_interpret_response_dap4_error() {
        string err = "<Error httpcode=\"400\"><Message>bad request</Message></Error>";
        assert_error([err] { interpret_response(result(err), "a.tif", 60); }, kBesError, "bad request");
    }

    void test_interpret_response_not_a_dmr() {
        RunResult r = result("something went wrong\n", 1);
        assert_error([r] { interpret_response(r, "a.tif", 60); }, kBesError,
                     "a.tif: BES did not return a DMR (exit status 1): something went wrong");
        r = result("", 0);
        r.err = "only on stderr";
        assert_error([r] { interpret_response(r, "a.tif", 60); }, kBesError, "only on stderr");
        assert_error([] { interpret_response(result(""), "a.tif", 60); }, kBesError, "no output from besstandalone");
        // A <Dataset> without the DAP4 namespace is not a DMR.
        assert_error([] { interpret_response(result("<Dataset name=\"x\"></Dataset>"), "a.tif", 60); }, kBesError,
                     "BES did not return a DMR");
    }

    void test_interpret_response_long_error_truncated() {
        RunResult r = result(string(5000, 'x'), 1);
        try {
            interpret_response(r, "a.tif", 60);
            CPPUNIT_FAIL("expected an error");
        }
        catch (const GeoToDMRInternalError &e) {
            CPPUNIT_ASSERT(string(e.what()).size() < 2200);
            CPPUNIT_ASSERT(contains(e.what(), " ..."));
        }
    }

    void test_interpret_response_signaled() {
        RunResult r = result("");
        r.signaled = true;
        r.signal = 11;
        assert_error([r] { interpret_response(r, "a.tif", 60); }, kBesError, "killed by signal 11");
    }

    void test_interpret_response_truncated_dmr() {
        string partial = "<Dataset xmlns=\"http://xml.opendap.org/ns/DAP/4.0#\" name=\"a.tif\">\n<Dimension";
        assert_error([partial] { interpret_response(result(partial), "a.tif", 60); }, kBesError,
                     "DMR response is truncated");
    }

    void test_interpret_response_timeout_and_exec_errors() {
        RunResult r = result("");
        r.timed_out = true;
        assert_error([r] { interpret_response(r, "a.tif", 9); }, kTimeout, "a.tif: besstandalone timed out after 9 s");
        assert_error([] { interpret_response(result("", 127), "a.tif", 60); }, kInternal,
                     "could not execute besstandalone");
        assert_error([] { interpret_response(result("", 126), "a.tif", 60); }, kInternal,
                     "could not set up the besstandalone process");
    }

    // ---- output_path_for and write_output --------------------------------------------

    void test_output_path_for() {
        CmdArgs args;
        CPPUNIT_ASSERT_EQUAL(string("/d/cea.tif.dmr"), output_path_for(args, "/d/cea.tif"));
        args.out_dir = "/out";
        CPPUNIT_ASSERT_EQUAL(string("/out/cea.tif.dmr"), output_path_for(args, "/d/cea.tif"));
        args.out_dir.clear();
        args.output = "x.dmr";
        CPPUNIT_ASSERT_EQUAL(string("x.dmr"), output_path_for(args, "/d/cea.tif"));
        args.output = "-";
        CPPUNIT_ASSERT_EQUAL(string("-"), output_path_for(args, "/d/cea.tif"));
    }

    void test_write_output_file() {
        string dest = in_work("new/sub/x.dmr");
        write_output(dest, "first");
        CPPUNIT_ASSERT_EQUAL(string("first"), read_file(dest));
        write_output(dest, "second");  // replaced
        CPPUNIT_ASSERT_EQUAL(string("second"), read_file(dest));
        CPPUNIT_ASSERT(!path_exists(dest + ".tmp." + to_string(::getpid())));
    }

    void test_write_output_relative_name() {
        CPPUNIT_ASSERT(::chdir(d_work.c_str()) == 0);
        write_output("plain.dmr", "x");
        CPPUNIT_ASSERT_EQUAL(string("x"), read_file(in_work("plain.dmr")));
    }

    void test_write_output_stdout() {
        string out = capture(cout, [] { write_output("-", "<Dataset/>\n"); });
        CPPUNIT_ASSERT_EQUAL(string("<Dataset/>\n"), out);
        CPPUNIT_ASSERT(!path_exists(in_work("-")));
    }

    void test_write_output_error() {
        make_dirs(in_work("isdir.dmr"));
        write_file(in_work("isdir.dmr/keep"), "x");  // rename() onto a non-empty dir fails
        assert_error([this] { write_output(in_work("isdir.dmr"), "x"); }, kInternal, "cannot write");
        CPPUNIT_ASSERT(!path_exists(in_work("isdir.dmr.tmp." + to_string(::getpid()))));
    }

    // ---- process_one -----------------------------------------------------------

    // A stand-in besstandalone that keeps copies of what it was given.
    string fake_bes_returning(const string &response) {
        write_file(in_work("response.txt"), response);
        return write_script("fake-bes", "cp \"$2\" " + in_work("seen.conf") + "; cp \"$4\" " + in_work("seen.bescmd") +
                                            "; cat " + in_work("response.txt"));
    }

    void test_process_one_writes_dmr() {
        string input = copy_data("cea.tif", "cea.tif");
        string exe = fake_bes_returning("junk\n" + string(kDmr) + "\n");
        CmdArgs args;
        args.extra_keys = {"X.Y=z"};
        process_one(args, input, exe, "/lib/bes");

        CPPUNIT_ASSERT_EQUAL(string(kDmr) + "\n", read_file(in_work("cea.tif.dmr")));
        string conf = read_file(in_work("seen.conf"));
        CPPUNIT_ASSERT(contains(conf, "BES.Catalog.catalog.RootDirectory=" + d_work + "\n"));
        CPPUNIT_ASSERT(contains(conf, "BES.module.gdal=/lib/bes/libgdal_module.so\n"));
        CPPUNIT_ASSERT(contains(conf, "X.Y=z\n"));
        CPPUNIT_ASSERT(contains(read_file(in_work("seen.bescmd")), ">/cea.tif</bes:setContainer>"));
    }

    void test_process_one_output_options() {
        string input = copy_data("cea.tif", "cea.tif");
        string exe = fake_bes_returning(kDmr);
        CmdArgs args;
        args.out_dir = in_work("out");
        process_one(args, input, exe, "/lib/bes");
        CPPUNIT_ASSERT(path_exists(in_work("out/cea.tif.dmr")));
        CPPUNIT_ASSERT(!path_exists(in_work("cea.tif.dmr")));

        args.out_dir.clear();
        args.output = "-";
        string out = capture(cout, [&] { process_one(args, input, exe, "/lib/bes"); });
        CPPUNIT_ASSERT_EQUAL(string(kDmr) + "\n", out);
    }

    void test_process_one_cleans_up_temp_dir() {
        string input = copy_data("cea.tif", "cea.tif");
        make_dirs(in_work("tmp"));
        setenv("TMPDIR", in_work("tmp").c_str(), 1);

        process_one(CmdArgs(), input, fake_bes_returning(kDmr), "/lib/bes");
        CPPUNIT_ASSERT(read_file(in_work("cea.tif.dmr")).size() > 0);

        string exe = fake_bes_returning("<BESError><Message>boom</Message></BESError>");
        assert_error([&] { process_one(CmdArgs(), input, exe, "/lib/bes"); }, kBesError, "boom");

        // Both runs removed their temp directories: rmdir() fails on a non-empty one.
        CPPUNIT_ASSERT_EQUAL(0, ::rmdir(in_work("tmp").c_str()));
    }

    void test_process_one_keep_temp() {
        string input = copy_data("cea.tif", "cea.tif");
        make_dirs(in_work("tmp"));
        setenv("TMPDIR", in_work("tmp").c_str(), 1);
        CmdArgs args;
        args.keep_temp = true;
        string err = capture(cerr, [&] { process_one(args, input, fake_bes_returning(kDmr), "/lib/bes"); });
        CPPUNIT_ASSERT(contains(err, "kept temp files in " + in_work("tmp/GeotiffToDmr.")));
        string kept = trim(err.substr(err.find(in_work("tmp/"))));
        CPPUNIT_ASSERT(path_exists(kept + "/bes.conf"));
        CPPUNIT_ASSERT(path_exists(kept + "/get_dmr.bescmd"));
        CPPUNIT_ASSERT(path_exists(kept + "/stdout.txt"));
    }

    void test_process_one_bad_input_does_not_run_bes() {
        string exe = write_script("fake-bes", "touch " + in_work("bes-ran"));
        string text = write_bytes("text.tif", "not a tiff at all");
        assert_error([&] { process_one(CmdArgs(), text, exe, "/lib/bes"); }, kBadInput, "bad magic bytes");
        CPPUNIT_ASSERT(!path_exists(in_work("bes-ran")));
        CPPUNIT_ASSERT(!path_exists(in_work("text.tif.dmr")));
    }

    void test_process_one_failure_leaves_old_dmr() {
        string input = copy_data("cea.tif", "cea.tif");
        write_file(in_work("cea.tif.dmr"), "previous");
        string exe = fake_bes_returning("<Dataset xmlns=\"http://xml.opendap.org/ns/DAP/4.0#\">");
        assert_error([&] { process_one(CmdArgs(), input, exe, "/lib/bes"); }, kBesError, "truncated");
        CPPUNIT_ASSERT_EQUAL(string("previous"), read_file(in_work("cea.tif.dmr")));
    }

    CPPUNIT_TEST_SUITE(GeotiffToDmrTest);

    CPPUNIT_TEST(test_path_join);
    CPPUNIT_TEST(test_path_dirname);
    CPPUNIT_TEST(test_path_basename);
    CPPUNIT_TEST(test_path_extension_lower);
    CPPUNIT_TEST(test_path_exists);
    CPPUNIT_TEST(test_path_absolute);
    CPPUNIT_TEST(test_make_dirs_and_remove_tree);
    CPPUNIT_TEST(test_make_dirs_error);
    CPPUNIT_TEST(test_read_write_file);
    CPPUNIT_TEST(test_write_file_error);
    CPPUNIT_TEST(test_xml_escape);
    CPPUNIT_TEST(test_trim);

    CPPUNIT_TEST(test_which_absolute_path);
    CPPUNIT_TEST(test_which_relative_path_made_absolute);
    CPPUNIT_TEST(test_which_finds_program_on_path);
    CPPUNIT_TEST(test_which_not_found);

    CPPUNIT_TEST(test_log_verbose);
    CPPUNIT_TEST(test_print_usage);

    CPPUNIT_TEST(test_parse_args_defaults);
    CPPUNIT_TEST(test_parse_args_all_options);
    CPPUNIT_TEST(test_parse_args_long_and_short_forms);
    CPPUNIT_TEST(test_parse_args_double_dash_and_inputs);
    CPPUNIT_TEST(test_parse_args_besstandalone_from_environment);
    CPPUNIT_TEST(test_parse_args_errors);

    CPPUNIT_TEST(test_temp_dir_created_and_removed);
    CPPUNIT_TEST(test_temp_dir_keep);
    CPPUNIT_TEST(test_temp_dir_unique);
    CPPUNIT_TEST(test_temp_dir_error);

    CPPUNIT_TEST(test_validate_input_geotiff);
    CPPUNIT_TEST(test_validate_input_returns_canonical_path);
    CPPUNIT_TEST(test_validate_input_extension_any_case);
    CPPUNIT_TEST(test_validate_input_errors);
    CPPUNIT_TEST(test_validate_input_tiff_magic_variants);
    CPPUNIT_TEST(test_validate_input_not_georeferenced);
    CPPUNIT_TEST(test_validate_input_unreadable);

    CPPUNIT_TEST(test_find_bes_lib_dir_explicit);
    CPPUNIT_TEST(test_find_bes_lib_dir_explicit_missing_module);
    CPPUNIT_TEST(test_find_bes_lib_dir_from_environment);
    CPPUNIT_TEST(test_compiled_default_lib_dir_is_set);
    CPPUNIT_TEST(test_find_bes_lib_dir_next_to_besstandalone);

    CPPUNIT_TEST(test_make_bes_conf_with_lib_dir);
    CPPUNIT_TEST(test_make_bes_conf_with_base_conf);
    CPPUNIT_TEST(test_make_bes_conf_extra_keys_come_last);
    CPPUNIT_TEST(test_make_bes_conf_needs_lib_dir_or_conf);
    CPPUNIT_TEST(test_make_bes_conf_type_match);
    CPPUNIT_TEST(test_make_bescmd);
    CPPUNIT_TEST(test_make_bescmd_escapes_file_name);

    CPPUNIT_TEST(test_run_captures_output_and_status);
    CPPUNIT_TEST(test_run_passes_arguments_and_directory);
    CPPUNIT_TEST(test_run_stdin_is_empty);
    CPPUNIT_TEST(test_run_timeout_kills_process_group);
    CPPUNIT_TEST(test_run_killed_by_signal);
    CPPUNIT_TEST(test_run_exec_failure);

    CPPUNIT_TEST(test_extract_tag);
    CPPUNIT_TEST(test_interpret_response_dmr);
    CPPUNIT_TEST(test_interpret_response_strips_surrounding_text);
    CPPUNIT_TEST(test_interpret_response_without_xml_declaration);
    CPPUNIT_TEST(test_interpret_response_dmr_despite_exit_status);
    CPPUNIT_TEST(test_interpret_response_bes_error);
    CPPUNIT_TEST(test_interpret_response_dap4_error);
    CPPUNIT_TEST(test_interpret_response_not_a_dmr);
    CPPUNIT_TEST(test_interpret_response_long_error_truncated);
    CPPUNIT_TEST(test_interpret_response_signaled);
    CPPUNIT_TEST(test_interpret_response_truncated_dmr);
    CPPUNIT_TEST(test_interpret_response_timeout_and_exec_errors);

    CPPUNIT_TEST(test_output_path_for);
    CPPUNIT_TEST(test_write_output_file);
    CPPUNIT_TEST(test_write_output_relative_name);
    CPPUNIT_TEST(test_write_output_stdout);
    CPPUNIT_TEST(test_write_output_error);

    CPPUNIT_TEST(test_process_one_writes_dmr);
    CPPUNIT_TEST(test_process_one_output_options);
    CPPUNIT_TEST(test_process_one_cleans_up_temp_dir);
    CPPUNIT_TEST(test_process_one_keep_temp);
    CPPUNIT_TEST(test_process_one_bad_input_does_not_run_bes);
    CPPUNIT_TEST(test_process_one_failure_leaves_old_dmr);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(GeotiffToDmrTest);

}  // namespace geotiff_to_dmr

int main(int argc, char *argv[]) {
    return bes_run_tests<geotiff_to_dmr::GeotiffToDmrTest>(argc, argv, "cerr") ? 0 : 1;
}
