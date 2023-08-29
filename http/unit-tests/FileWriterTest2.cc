//
// James Gallagher on 3/11/23.
//
// This program demonstrates the need to use a class mutex to protect access to the filesystem.
// See the FileWriter class for details. Run the program using 'class' to use the class mutex,
// and 'instance' to use the instance mutex. The program creates two threads, each of which
// creates a FileWriter instance. Each thread will try to make a child directory in the CWD.
// THe code will work if the class mutex is used, but will fail if the instance mutex is used.
// If the directory already exists, so there is no race condition with the access() call in the
// FileWriter::mkdir_helper() method, the program will work with either mutex. jhrg 3/11/23
//
// Regarding the notion of making the directory in a thread safe way, it is possible to call
// mkdir(3) and ignore an error when the error is EEXIST. That is probably faster than using a
// mutex, but it is hard to use errno in MT code in a way that safe.
//
// Build using "g++ --std=c++14 -O3 -o FileWriterTest2 FileWriterTest2.cc"

#include <string>
#include <vector>
#include <iostream>
#include <future>

#include "FileWriter.h"

std::string getcwd()
{
    char cwd[1024]; // buffer to store the current working directory
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        return {cwd}; // convert C-style string to C++ string
    }
    else {
        throw std::runtime_error("Error: getcwd() failed: " + std::string(strerror(errno)));
    }
}

int main(int argc, char *argv[]) {
    bool use_class_mutex = false;
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <mutex>" << std::endl;
        return 1;
    }
    else {
        std::string mutex = argv[1];
        if (mutex == "class") {
            FileWriter::set_class_mutex(true);
        }
        else if (mutex == "instance") {
            FileWriter::set_class_mutex(false);
        }
        else {
            std::cerr << "Usage: " << argv[0] << " <mutex = class|instance>" << std::endl;
            return 1;
        }
    }

    try {
        // Create two instances of the FileWriter class, both associated with the same file
        FileWriter writer1("file.txt");
        FileWriter writer2("file.txt");

        std::vector<std::future<std::string>> futures;

        // Define two threads, each of which writes to the same file using a different
        // instance of the FileWriter class. The writer part of this is not really the
        // point - it's nice, but the C++ iostreams seem to be thread safe. The point is
        // to show that the mkdir() system call can only be used in a thread safe way by
        // using a class (static) mutex. The critical region of the call must be protected
        // by the same mutex when using _different_ instances of the same class. That can
        // be done by making the mutex static, jhrg 3/11/23
        futures.emplace_back(std::async(std::launch::async, [&writer1]() -> std::string {
            writer1.mkdir(getcwd() + "/dir", 0775);
            for (int i = 0; i < 7; i++) {
                writer1.write("Thread 1 writing to file\n");
            }
            return {"Thread 1 done"};
        }));

        futures.emplace_back(std::async(std::launch::async, [&writer2]() -> std::string {
            writer2.mkdir(getcwd() + "/dir", 0775);
            for (int i = 0; i < 11; i++) {
                writer2.write("Thread 2 writing to file\n");
            }
            return "Thread 2 done";
        }));

        for (auto &future: futures) {
            std::string result = future.get();
            std::cerr << result << std::endl;
        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    system("rm -rf dir");

    return 0;
}
