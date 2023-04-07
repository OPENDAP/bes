//
// Created by James Gallagher on 3/11/23.
// g++ --std=c++14 -o FileWriterTest1 FileWriterTest1.cc

#include <thread>
#include "FileWriter.h"

int main() {
    // Create two instances of the FileWriter class
    FileWriter writer1("file1.txt");
    FileWriter writer2("file2.txt");

    // Define two threads, each of which writes to a different file using a different instance of the FileWriter class
    std::thread t1([&writer1]() {
        for (int i = 0; i < 10; i++) {
            writer1.write("Thread 1 writing to file 1\n");
        }
    });

    std::thread t2([&writer2]() {
        for (int i = 0; i < 10; i++) {
            writer2.write("Thread 2 writing to file 2\n");
        }
    });

    // Wait for the threads to complete
    t1.join();
    t2.join();

    return 0;
}
