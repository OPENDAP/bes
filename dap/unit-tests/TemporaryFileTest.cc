// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
#include <test_config.h>
#include <signal.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>


#include "TempFile.h"
#include "BESHandlerUtil.h"
#include "BESInternalError.h"
#include "BESInternalFatalError.h"
#include "TheBESKeys.h"


#include <debug.h>

static bool debug = false;
static bool debug_2 = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);
#undef DBG2
#define DBG2(x) do { if (debug_2) (x); } while(false);

using namespace CppUnit;
using namespace std;

string temp_dir = string(TEST_SRC_DIR) + "/tmp_dir";
string tmp_template = temp_dir + "/tmp_XXXXXX";

class TemporaryFileTest: public CppUnit::TestFixture {
private:

public:
    TemporaryFileTest()
    {
    }

    ~TemporaryFileTest()
    {
    }

    void setUp()
    {
        DBG2(cerr <<  __func__ << "() - BEGIN" << endl);
        // Because TemporaryFile uses the BESLog macro ERROR we have to configure the BESKeys with the
        // BES config file name so that there is a log file name... Oy.
        string bes_conf = TEST_SRC_DIR;
        bes_conf.append("/bes.conf");
        TheBESKeys::ConfigFile = bes_conf;
        DBG(cerr <<  __func__ << "() - Temp file template is: '" << tmp_template << "'"  << endl);
        DBG2(cerr <<  __func__ << "() - END" << endl);
    }

    void tearDown()
    {
    }


    void normal_test()
    {
        std::string tmp_file_name;

        try {
            bes::TempFile tf(tmp_template);
            tmp_file_name = tf.get_name();
            DBG(cerr <<  __func__ << "() - Temp file is: '" << tmp_file_name << "' has been created. fd: "<< tf.get_fd()  << endl);

            // Is it really there? Just sayin'...
            struct stat buf;
            int statret = stat(tmp_file_name.c_str(), &buf);
            CPPUNIT_ASSERT(statret == 0);
        }
        catch (BESInternalError &bie) {
            DBG(cerr <<  __func__ << "() - Caught BESInternalError! Message: "<< bie.get_message()  << endl);
            CPPUNIT_ASSERT(false);
        }
        // The name should be set to something.
        CPPUNIT_ASSERT(tmp_file_name.length()>0);

        struct stat buf;
        int statret = stat(tmp_file_name.c_str(), &buf);
        // And the file should be gone because the class TemporaryFile went out of scope.
        CPPUNIT_ASSERT(statret != 0);
        DBG(cerr <<  __func__ << "() - Temp file '" << tmp_file_name << "' has been removed. (Temporary file out of scope)"  << endl);


    }

    void multi_file_normal_test()
    {
        int count=3;
        bes::TempFile *tfiles[count];
        string tmp_file_names[count];

        try {
            for(int i=0; i<count;i++){
                tfiles[i] = new bes::TempFile(tmp_template);
                tmp_file_names[i] = tfiles[i]->get_name();
                DBG(cerr <<  __func__ << "() - Temp file is: '" << tmp_file_names[i] << "' has been created. fd: "<< tfiles[i]->get_fd()  << endl);

                // Is it really there? Just sayin'...
                struct stat buf;
                int statret = stat(tmp_file_names[i].c_str(), &buf);
                CPPUNIT_ASSERT(statret == 0);
            }

            for(int i=0; i<count;i++){
                delete tfiles[i];
            }
        }
        catch (BESInternalError &bie) {
            DBG(cerr <<  __func__ << "() - Caught BESInternalError! Message: "<< bie.get_message()  << endl);
            CPPUNIT_ASSERT(false);
        }

        for(int i=0; i<count;i++){
            // The name should be set to something.
            CPPUNIT_ASSERT(tmp_file_names[i].length()>0);

            struct stat buf;
            int statret = stat(tmp_file_names[i].c_str(), &buf);
            // And the file should be gone because the class TemporaryFile went out of scope.
            CPPUNIT_ASSERT(statret != 0);
            DBG(cerr <<  __func__ << "() - Temp file '" << tmp_file_names[i] << "' has been removed. (Temporary file out of scope)"  << endl);
        }



    }

    void exception_test()
    {
        std::string tmp_file_name;

        try {
            bes::TempFile tf(tmp_template);
            tmp_file_name = tf.get_name();
            DBG(cerr <<  __func__ << "() - Temp file is: '" << tmp_file_name << "' has been created. fd: "<< tf.get_fd()  << endl);

            // Is it really there? Just sayin'...
            struct stat buf;
            int statret = stat(tmp_file_name.c_str(), &buf);
            CPPUNIT_ASSERT(statret == 0);

            throw BESInternalError("Throwing an exception to challenge the lifecycle...", __FILE__, __LINE__);


        }
        catch (BESInternalError &bie) {
            DBG(cerr <<  __func__ << "() - Caught BESInternalError (Expected)  Message: "<< bie.get_message()  << endl);
        }
        // The name should be set to something.
        CPPUNIT_ASSERT(tmp_file_name.length()>0);

        struct stat buf;
        int statret = stat(tmp_file_name.c_str(), &buf);
        // And the file should be gone because the class TemporaryFile went out of scope.
        CPPUNIT_ASSERT(statret != 0);
        DBG(cerr <<  __func__ << "() - Temp file '" << tmp_file_name << "' has been removed. (Temporary file out of scope)"  << endl);


    }

    static void register_sigpipe_handler()
    {
        struct sigaction act;
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGPIPE);
        act.sa_flags = 0;

        act.sa_handler = bes::TempFile::sigpipe_handler;
        if (sigaction(SIGPIPE, &act, 0)) {
            cerr << "Could not register a handler to catch SIGPIPE." << endl;
            exit(1);
        }
    }



    void sigpipe_test()
    {
        // Because we are going to fork and the child will be making a temp file, we use shared memory to
        // allow the parent to know the resulting file name.
        char *glob_name;
        int name_size = tmp_template.length() + 1;
        glob_name = (char *) mmap(NULL, name_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

        pid_t pid = fork();
        CPPUNIT_ASSERT(pid >= 0); // Make sure it didn't fail.

        if (pid){
            // parent - wait for the client to get sorted
            sleep(1);
            // Send child a the signal
            DBG(cerr <<  __func__ << "-PARENT() - Sending SIGPIPE to client."<< endl);
            kill(pid,SIGPIPE);
            // wait for the child to die.
            sleep(1);
            DBG(cerr <<  __func__ << "-PARENT() - Client should be dead. Temporary File Name: '" << glob_name << "'"<< endl);

            // Is it STILL there? Better not be...
            struct stat buf;
            int statret = stat(glob_name, &buf);
            CPPUNIT_ASSERT(statret != 0);
            DBG(cerr <<  __func__ << "-PARENT() - Temporary File: '" << glob_name  << "' was successfully removed. woot."<< endl);

            // Tidy up the shared memory business
            munmap(glob_name, name_size);

        }
        else {
            //child - register a signal handler
            //register_sigpipe_handler();

            std::string tmp_file_name;
            try {
                DBG(cerr <<  __func__ << "-CHILD() - Creating temporary file." << endl);
                bes::TempFile tf(tmp_template);
                tmp_file_name = tf.get_name();
                DBG(cerr <<  __func__ << "-CHILD() - Temp file is: '" << tmp_file_name << "' has been created. fd: "<< tf.get_fd()  << endl);
                // copy the filename into shared memory.
                tmp_file_name.copy(glob_name, tmp_file_name.size(), 0);

                // Is it really there? Just sayin'...
                struct stat buf;
                int statret = stat(tmp_file_name.c_str(), &buf);
                CPPUNIT_ASSERT(statret == 0);
                // Wait for the signal
                sleep(100);
                DBG(cerr <<  __func__ << "-CHILD() - Client is Alive." << endl);
            }
            catch (BESInternalError &bie) {
                DBG(cerr <<  __func__ << "-CHILD() - Caught BESInternalError  Message: "<< bie.get_message()  << endl);
                CPPUNIT_ASSERT(false);
            }
            DBG(cerr <<  __func__ << "-CHILD() - Client is exiting normally." << endl);

        }


    }

    void multifile_sigpipe_test()
    {
        // Because we are going to fork and the child will be making a temp file, we use shared memory to
        // allow the parent to know the resulting file names.
        char *glob_name[3];
        int name_size = tmp_template.length() + 1;
        glob_name[0] = (char *) mmap(NULL, name_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        glob_name[1] = (char *) mmap(NULL, name_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        glob_name[2] = (char *) mmap(NULL, name_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

        pid_t pid = fork();
        CPPUNIT_ASSERT(pid >= 0); // Make sure it didn't fail.

        if (pid){
            // parent - wait for the client to get sorted
            sleep(1);
            // Send child a the signal
            DBG(cerr <<  __func__ << "-PARENT() - Sending SIGPIPE to client."<< endl);
            kill(pid,SIGPIPE);
            // wait for the child to die.
            sleep(1);
            DBG(cerr <<  __func__ << "-PARENT() - Client should be dead."<< endl);

            for (int i=0; i<3 ;i++){
                // Is it STILL there? Better not be...
                struct stat buf;
                int statret = stat(glob_name[i], &buf);
                CPPUNIT_ASSERT(statret != 0);
                DBG(cerr <<  __func__ << "-PARENT() - Temporary File: '" << glob_name[i]  << "' was successfully removed. woot."<< endl);
            }

            // Tidy up the shared memory business
            munmap(glob_name, name_size);

        }
        else {
            //child - register a signal handler
            //register_sigpipe_handler();

            std::string tmp_file_name;
            struct stat buf;
            int statret;
            try {

                // --------- File One ------------
                DBG(cerr <<  __func__ << "-CHILD() - Creating temporary file." << endl);
                bes::TempFile tf1(tmp_template);
                tmp_file_name = tf1.get_name();
                DBG(cerr <<  __func__ << "-CHILD() - Temp file is: '" << tmp_file_name << "' has been created. fd: "<< tf1.get_fd()  << endl);
                // copy the filename into shared memory.
                tmp_file_name.copy(glob_name[0], tmp_file_name.size(), 0);

                // Is it really there? Just sayin'...
                statret = stat(tmp_file_name.c_str(), &buf);
                CPPUNIT_ASSERT(statret == 0);

                // --------- File Two ------------
                DBG(cerr <<  __func__ << "-CHILD() - Creating temporary file." << endl);
                bes::TempFile tf2(tmp_template);
                tmp_file_name = tf2.get_name();
                DBG(cerr <<  __func__ << "-CHILD() - Temp file is: '" << tmp_file_name << "' has been created. fd: "<< tf2.get_fd()  << endl);
                // copy the filename into shared memory.
                tmp_file_name.copy(glob_name[1], tmp_file_name.size(), 0);

                // Is it really there? Just sayin'...
                statret = stat(tmp_file_name.c_str(), &buf);
                CPPUNIT_ASSERT(statret == 0);

                // --------- File Three ------------
                DBG(cerr <<  __func__ << "-CHILD() - Creating temporary file." << endl);
                bes::TempFile tf3(tmp_template);
                tmp_file_name = tf3.get_name();
                DBG(cerr <<  __func__ << "-CHILD() - Temp file is: '" << tmp_file_name << "' has been created. fd: "<< tf3.get_fd()  << endl);
                // copy the filename into shared memory.
                tmp_file_name.copy(glob_name[2], tmp_file_name.size(), 0);

                // Is it really there? Just sayin'...
                statret = stat(tmp_file_name.c_str(), &buf);
                CPPUNIT_ASSERT(statret == 0);


                // Wait for the signal
                sleep(100);
                DBG(cerr <<  __func__ << "-CHILD() - Client is Alive." << endl);
            }
            catch (BESInternalError &bie) {
                DBG(cerr <<  __func__ << "-CHILD() - Caught BESInternalError  Message: "<< bie.get_message()  << endl);
                CPPUNIT_ASSERT(false);
            }
            DBG(cerr <<  __func__ << "-CHILD() - Client is exiting normally." << endl);

        }


    }



    CPPUNIT_TEST_SUITE( TemporaryFileTest );

    CPPUNIT_TEST(normal_test);
    CPPUNIT_TEST(multi_file_normal_test);
    CPPUNIT_TEST(exception_test);
    CPPUNIT_TEST(sigpipe_test);
    CPPUNIT_TEST(multifile_sigpipe_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TemporaryFileTest);

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dDh");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'D':
            debug_2 = 1;
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: TemporaryFileTest has the following tests:" << endl;
            const std::vector<Test*> &tests = TemporaryFileTest::suite()->getTests();
            unsigned int prefix_len = TemporaryFileTest::suite()->getName().append("::").length();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
            }
            break;
        }
        default:
            break;
        }

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = TemporaryFileTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            i++;
        }
    }

    return wasSuccessful ? 0 : 1;
}

