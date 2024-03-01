// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <csignal>
#include <cstdlib>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <unistd.h>

#include <libdap/debug.h>

#include "BESInternalError.h"
#include "BESInternalFatalError.h"
#include "TheBESKeys.h"
#include "BESUtil.h"
#include "BESDebug.h"
#include "TempFile.h"
#include "modules/common/run_tests_cppunit.h"

#include "test_config.h"

using namespace CppUnit;
using namespace std;

namespace bes {

class TemporaryFileTest : public CppUnit::TestFixture {
    const string TEMP_DIR = string(TEST_BUILD_DIR).append("/temp_file_test");
    const string TEMP_FILE_PREFIX = "tmpf_test";
    const string BES_CONF_FILE = BESUtil::assemblePath(TEST_BUILD_DIR, "bes.conf");

    static bool file_present(const string &file_name) {
        struct stat buf{};
        return (stat(file_name.c_str(), &buf) == 0);
    }

public:
    TemporaryFileTest() = default;

    ~TemporaryFileTest() = default;

    void setUp() override {
        DBG2(cerr << __func__ << "() - BEGIN" << endl);

        if (debug2) BESDebug::SetUp("cerr,dap");

        // Because TemporaryFile uses the BESLog macro ERROR we have
        // to configure the BESKeys with the BES config file name so
        // that there is a log file name... Oy.
        TheBESKeys::ConfigFile = BES_CONF_FILE;
        DBG(cerr << __func__ << "() - Temp file template is: '" << TEMP_FILE_PREFIX << "'" << endl);
        DBG2(cerr << __func__ << "() - END" << endl);
    }

    void mk_temp_dir_normal() {
        TempFile tf;
        auto tmp_dir = string(TEST_BUILD_DIR) + "/mk_temp_dir_normal";
        CPPUNIT_ASSERT_MESSAGE("The directory should not exist", access(tmp_dir.c_str(), F_OK) == -1);
        CPPUNIT_ASSERT_MESSAGE("The directory should not exist and should be created", tf.mk_temp_dir(tmp_dir));
        CPPUNIT_ASSERT_MESSAGE("The directory should exist", access(tmp_dir.c_str(), F_OK) == 0);
        rmdir(tmp_dir.c_str());
        CPPUNIT_ASSERT_MESSAGE("The directory should not exist", access(tmp_dir.c_str(), F_OK) == -1);
    }

    void mk_temp_dir_exists() {
        TempFile tf;
        auto tmp_dir = string(TEST_BUILD_DIR) + "/mk_temp_dir_normal";
        CPPUNIT_ASSERT_MESSAGE("The directory should exist", access(TEMP_DIR.c_str(), F_OK) == 0);
        CPPUNIT_ASSERT_MESSAGE("The directory should exist and should not be created", !tf.mk_temp_dir(TEMP_DIR));
        CPPUNIT_ASSERT_MESSAGE("The directory should still exist", access(TEMP_DIR.c_str(), F_OK) == 0);
    }

    // Grasping at straws here; the code should not be able to make a temporary directory in root (/)
    void mk_temp_dir_not_allowed() {
        TempFile tf;
        string tmp_dir = "/temporary_directory";
        CPPUNIT_ASSERT_MESSAGE("The directory should not exist", access(tmp_dir.c_str(), F_OK) == -1);
        CPPUNIT_ASSERT_THROW_MESSAGE("The directory should not exist and should not be created",
                                        tf.mk_temp_dir(tmp_dir), BESInternalFatalError);
        CPPUNIT_ASSERT_MESSAGE("The directory should not exist", access(tmp_dir.c_str(), F_OK) == -1);
    }

    void create_normal_test() {
        std::string tmp_file_name;

        try {
            TempFile tf;
            tmp_file_name = tf.create(TEMP_DIR, TEMP_FILE_PREFIX);
            DBG(cerr << __func__ << "() - Temp file is: '" << tmp_file_name << "' has been created. fd: "
                     << tf.get_fd() << endl);

            // Is it really there? Just sayin'...
            CPPUNIT_ASSERT_MESSAGE("The temporary file should exist", file_present(tmp_file_name));

            CPPUNIT_ASSERT_MESSAGE("TempFile.open_files should have one entry", tf.open_files.size() == 1);
        }
        catch (BESInternalError &bie) {
            DBG(cerr << __func__ << "() - Caught BESInternalError! Message: " << bie.get_message() << endl);
            CPPUNIT_ASSERT(false);
        }

        // And the file should be gone because the class TemporaryFile went out of scope.
        CPPUNIT_ASSERT_MESSAGE("The file should not exist since the temporary file scope was exited.", !file_present(tmp_file_name));

        // The name should be set to something.
        CPPUNIT_ASSERT_MESSAGE("Even after deletion, this name should not be empty", !tmp_file_name.empty());
    }

    void create_multi_file_normal_test() {
        int count = 3;
        vector<unique_ptr<TempFile>> tfiles(count);
        vector<string> tmp_file_names(count);

        try {
            for (int i = 0; i < count; i++) {

                tfiles[i] = make_unique<TempFile>();
                tmp_file_names[i] = tfiles[i]->create(TEMP_DIR, TEMP_FILE_PREFIX);

                DBG(cerr << __func__ << "() - Temp file is: '" << tmp_file_names[i] << "' has been created. fd: "
                         << tfiles[i]->get_fd() << endl);

                CPPUNIT_ASSERT_MESSAGE("Temporary file should exist.", file_present(tfiles[i]->get_name()));
            }

            CPPUNIT_ASSERT_MESSAGE("TempFile.open_files should have three entries", tfiles[0]->open_files.size() == 3);
            CPPUNIT_ASSERT_MESSAGE("TempFile.open_files should have three entries", tfiles[1]->open_files.size() == 3);
            CPPUNIT_ASSERT_MESSAGE("TempFile.open_files should have three entries", tfiles[2]->open_files.size() == 3);
            tfiles.clear();
        }
        catch (const BESInternalError &bie) {
            DBG(cerr << __func__ << "() - Caught BESInternalError! Message: " << bie.get_message() << endl);
            CPPUNIT_FAIL("Caught BESInternalError! Message: " + bie.get_message());
        }

        for (int i = 0; i < count; i++) {
            // The name should be set to something.
            CPPUNIT_ASSERT(!tmp_file_names[i].empty());

            // And the file should be gone because the class TemporaryFile went out of scope.
            DBG(cerr << __func__ << "() - Temp file '" << tmp_file_names[i]
                     << "' has been removed. (Temporary file out of scope)" << endl);

            CPPUNIT_ASSERT_MESSAGE("The temporary files should be deleted", !file_present(tmp_file_names[i]));
        }
    }

    // Test that some of the TempFile objects can be removed/deleted and the 'open_files'
    // map is still around.
    void create_multi_file_normal_test_2() {
        int count = 3;
        vector<unique_ptr<TempFile>> tfiles(count);
        vector<string> tmp_file_names(count);

        try {
            for (int i = 0; i < count; i++) {

                tfiles[i] = make_unique<TempFile>();
                tmp_file_names[i] = tfiles[i]->create(TEMP_DIR, TEMP_FILE_PREFIX);

                DBG(cerr << __func__ << "() - Temp file is: '" << tmp_file_names[i] << "' has been created. fd: "
                         << tfiles[i]->get_fd() << endl);

                CPPUNIT_ASSERT_MESSAGE("Temporary file should exist.", file_present(tfiles[i]->get_name()));
            }

            CPPUNIT_ASSERT_MESSAGE("tfiles should have three elements.", tfiles.size() == 3);
            CPPUNIT_ASSERT_MESSAGE("TempFile.open_files should have three entries", tfiles[0]->open_files.size() == 3);
            CPPUNIT_ASSERT_MESSAGE("TempFile.open_files should have three entries", tfiles[1]->open_files.size() == 3);
            CPPUNIT_ASSERT_MESSAGE("TempFile.open_files should have three entries", tfiles[2]->open_files.size() == 3);
            tfiles.erase(tfiles.begin(), tfiles.begin() + 2);
            CPPUNIT_ASSERT_MESSAGE("tfiles should have one element, not " + to_string(tfiles.size()), tfiles.size() == 1);
            CPPUNIT_ASSERT_MESSAGE("The first temporary file should be gone", !file_present(tmp_file_names[0]));
            CPPUNIT_ASSERT_MESSAGE("The second temporary file should be gone", !file_present(tmp_file_names[1]));
        }
        catch (const BESInternalError &bie) {
            DBG(cerr << __func__ << "() - Caught BESInternalError! Message: " << bie.get_message() << endl);
            CPPUNIT_FAIL("Caught BESInternalError! Message: " + bie.get_message());
        }

        tfiles.clear();

        CPPUNIT_ASSERT_MESSAGE("The third temporary file should be gone now", !file_present(tmp_file_names[2]));
    }

    void create_exception_test() {
        string tmp_file_name;

        try {
            bes::TempFile tf;
            tmp_file_name = tf.create(TEMP_DIR, TEMP_FILE_PREFIX);
            DBG(cerr << __func__ << "() - Temp file is: '" << tmp_file_name << "' has been created. fd: " << tf.get_fd()
                     << endl);

            // Is it really there? Just sayin'...
            CPPUNIT_ASSERT(file_present(tmp_file_name));

            throw BESInternalError("Throwing an exception to challenge the lifecycle...", __FILE__, __LINE__);
        }
        catch (BESInternalError &bie) {
            DBG(cerr << __func__ << "() - Caught BESInternalError (Expected)  Message: " << bie.get_message() << endl);
        }

        // The name should be set to something.
        CPPUNIT_ASSERT(!tmp_file_name.empty());

        // And the file should be gone because the class TemporaryFile went out of scope.
        DBG(cerr << __func__ << "() - Temp file '" << tmp_file_name
                 << "' has been removed. (Temporary file out of scope)" << endl);
        CPPUNIT_ASSERT(!file_present(tmp_file_name));
    }

    void sigpipe_test() {
        // Because we are going to fork and the child will be making a temp file, we use shared memory to
        // allow the parent to know the resulting file name.
        char *glob_name;
        auto name_size = TEMP_FILE_PREFIX.size() + 1;
        glob_name = (char *) mmap(nullptr, name_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

        pid_t pid = fork();
        CPPUNIT_ASSERT(pid >= 0); // Make sure it didn't fail.

        if (pid) {
            // parent - wait for the client to get sorted
            sleep(1);

            CPPUNIT_ASSERT_MESSAGE("The file " + string(glob_name) + " should be present.", file_present(glob_name));

            // Send child the signal
            DBG(cerr << __func__ << "-PARENT() - Sending SIGPIPE to child process." << endl);
            kill(pid, SIGPIPE);

            // wait for the child process to catch the signal, remove the temp file and exit.
            int status;
            auto child_pid = waitpid(pid, &status, 0);
            DBG(cerr << __func__ << "-PARENT() - Child exited at this point,  status: " << status << endl);
            CPPUNIT_ASSERT_MESSAGE("The child process should have exited.", child_pid == pid);
            CPPUNIT_ASSERT_MESSAGE("The child process should exit with a status of SIGPIPE, was: " + to_string(status), status == SIGPIPE);

            // Is it STILL there? Better not be...
            CPPUNIT_ASSERT_MESSAGE("The file " + string(glob_name) + " should be deleted.", !file_present(glob_name));
            DBG(cerr << __func__ << "-PARENT() - Temporary File: '" << glob_name << "' was successfully removed. woot."
                     << endl);

            // Tidy up the shared memory business
            munmap(glob_name, name_size);
        }
        else {
            // child
            std::string tmp_file_name;
            try {
                DBG(cerr << __func__ << "-CHILD() - Creating temporary file." << endl);
                bes::TempFile tf;
                tmp_file_name = tf.create(TEMP_DIR, TEMP_FILE_PREFIX);
                DBG(cerr << __func__ << "-CHILD() - Temp file is: '" << tmp_file_name << "' has been created. fd: "
                         << tf.get_fd() << endl);

                // copy the filename into shared memory.
                tmp_file_name.copy(glob_name, tmp_file_name.size(), 0);

                // Is it really there? Just sayin'...
                CPPUNIT_ASSERT(file_present(tmp_file_name));

                // Wait for the signal - SIGPIPE should cause the process to exit
                sleep(100);
                DBG(cerr << __func__ << "-CHILD() - Client process is Alive." << endl);
                CPPUNIT_FAIL("The child process should have exited before this point.");
            }
            catch (BESInternalError &bie) {
                DBG(cerr << __func__ << "-CHILD() - Caught BESInternalError  Message: " << bie.get_message() << endl);
                CPPUNIT_FAIL("Caught BESInternalError! Message: " + bie.get_message());
            }

            DBG(cerr << __func__ << "-CHILD() - Client is exiting normally." << endl);
            CPPUNIT_FAIL("The child process should have exited before this point.");
        }
    }

    void multifile_sigpipe_test() {
        // Because we are going to fork and the child will be making a temp file, we use shared memory to
        // allow the parent to know the resulting file names.
        char *glob_name[3];
        auto name_size = TEMP_FILE_PREFIX.size() + 1;
        glob_name[0] = (char *) mmap(nullptr, name_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        glob_name[1] = (char *) mmap(nullptr, name_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        glob_name[2] = (char *) mmap(nullptr, name_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

        pid_t pid = fork();
        CPPUNIT_ASSERT(pid >= 0); // Make sure it didn't fail.

        if (pid) {
            // parent - wait for the client to get sorted
            sleep(1);

            for (const char *name : glob_name) {
                 CPPUNIT_ASSERT_MESSAGE("The file " + string(name) + " should be present.", file_present(name));
            }

            // Send the child process the signal
            DBG(cerr << __func__ << "-PARENT() - Sending SIGPIPE to child process." << endl);
            kill(pid, SIGPIPE);

            // wait for the child process to exit.
            int status;
            auto child_pid = waitpid(pid, &status, 0);
            DBG(cerr << __func__ << "-PARENT() - Child exited at this point,  status: " << status << endl);
            CPPUNIT_ASSERT_MESSAGE("The child process should have exited.", child_pid == pid);
            CPPUNIT_ASSERT_MESSAGE("The child process should exit with a status of SIGPIPE, was: " + to_string(status), status == SIGPIPE);


            for (const char *name : glob_name) {
                CPPUNIT_ASSERT_MESSAGE("The file " + string(name) + " should not be present.", !file_present(name));
                DBG(cerr << __func__ << "-PARENT() - Temporary File: '" << name
                         << "' was successfully removed. woot." << endl);
            }

            // Tidy up the shared memory business
            munmap(glob_name, name_size);
        }
        else {
            //child - register a signal handler
            std::string tmp_file_name;
            try {
                // --------- File One ------------
                DBG(cerr << __func__ << "-CHILD() - Creating temporary file." << endl);
                bes::TempFile tf1;
                tmp_file_name = tf1.create(TEMP_DIR, TEMP_FILE_PREFIX);
                DBG(cerr << __func__ << "-CHILD() - Temp file is: '" << tmp_file_name << "' has been created. fd: "
                         << tf1.get_fd() << endl);
                // copy the filename into shared memory.
                tmp_file_name.copy(glob_name[0], tmp_file_name.size(), 0);

                CPPUNIT_ASSERT_MESSAGE("The file " + tmp_file_name + " should be present.", file_present(tmp_file_name));

                // --------- File Two ------------
                DBG(cerr << __func__ << "-CHILD() - Creating temporary file." << endl);
                bes::TempFile tf2;
                tmp_file_name = tf2.create(TEMP_DIR, TEMP_FILE_PREFIX);
                DBG(cerr << __func__ << "-CHILD() - Temp file is: '" << tmp_file_name << "' has been created. fd: "
                         << tf2.get_fd() << endl);
                // copy the filename into shared memory.
                tmp_file_name.copy(glob_name[1], tmp_file_name.size(), 0);

                CPPUNIT_ASSERT_MESSAGE("The file " + tmp_file_name + " should be present.", file_present(tmp_file_name));

                // --------- File Three ------------
                DBG(cerr << __func__ << "-CHILD() - Creating temporary file." << endl);
                bes::TempFile tf3;
                tmp_file_name = tf3.create(TEMP_DIR, TEMP_FILE_PREFIX);
                DBG(cerr << __func__ << "-CHILD() - Temp file is: '" << tmp_file_name << "' has been created. fd: "
                         << tf3.get_fd() << endl);
                // copy the filename into shared memory.
                tmp_file_name.copy(glob_name[2], tmp_file_name.size(), 0);

                CPPUNIT_ASSERT_MESSAGE("The file " + tmp_file_name + " should be present.", file_present(tmp_file_name));

                // Wait for the signal
                sleep(100);
                DBG(cerr << __func__ << "-CHILD() - Client is Alive." << endl);
                CPPUNIT_FAIL("The child process should have exited before this point.");
            }
            catch (BESInternalError &bie) {
                DBG(cerr << __func__ << "-CHILD() - Caught BESInternalError  Message: " << bie.get_message() << endl);
                CPPUNIT_ASSERT(false);
            }

            DBG(cerr << __func__ << "-CHILD() - Client is exiting normally." << endl);
            CPPUNIT_FAIL("The child process should have exited before this point.");
        }
    }

CPPUNIT_TEST_SUITE(TemporaryFileTest);

        CPPUNIT_TEST(mk_temp_dir_normal);
        CPPUNIT_TEST(mk_temp_dir_exists);
        CPPUNIT_TEST(mk_temp_dir_not_allowed);

        CPPUNIT_TEST(create_normal_test);
        CPPUNIT_TEST(create_multi_file_normal_test);
        CPPUNIT_TEST(create_multi_file_normal_test_2);
        CPPUNIT_TEST(create_exception_test);

        CPPUNIT_TEST(sigpipe_test);
        CPPUNIT_TEST(multifile_sigpipe_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TemporaryFileTest);

} // namespace bes

int main(int argc, char*argv[]) {
    return bes_run_tests<bes::TemporaryFileTest>(argc, argv, "dDh") ? 0 : 1;
}

#if 0
int option_char;
    while ((option_char = getopt(argc, argv, "dDh")) != -1)
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            break;
        case 'D':
            debug_2 = true;
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: TemporaryFileTest has the following tests:" << endl;
            const std::vector<Test*> &tests = TemporaryFileTest::suite()->getTests();
            unsigned int prefix_len = bes::TemporaryFileTest::suite()->getName().append("::").size();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
            }
            break;
        }
        default:
            break;
        }

    argc -= optind;
    argv += optind;

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = TemporaryFileTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            i++;
        }
    }

    return wasSuccessful ? 0 : 1;
}
#endif
