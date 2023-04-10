//
// Created by James Gallagher on 3/11/23.
//

#ifndef BES_FILEWRITER_H
#define BES_FILEWRITER_H

#include <mutex>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>
#include <sys/stat.h>

class FileWriter {
public:
    FileWriter(const std::string& filename) : filename_(filename) { }

    static void set_class_mutex(bool use_class_mutex) {
        use_class_mutex_ = use_class_mutex;
    }
    void write(const std::string& data) {
        // std::lock_guard<std::mutex> lock(mutex_);
        // Even though chatGPT sez we need a mutex here, it appears that the fstream
        // class is already thread safe. jhrg 3/11/23
        file_.open(filename_, std::ios::out | std::ios::app);
        file_ << "Info: " << data;
        file_.close();
    }

    // This shows that the class (static) mutex is needed to lock access to the
    // filesystem. The instance mutex is not sufficient. jhrg 3/11/23
    void mkdir(const std::string& path, mode_t mode) {//}, bool use_class_mutex = false) {
        if (use_class_mutex_) {
            std::lock_guard<std::mutex> lock(class_mutex_);
            mkdir_helper(path, mode);
        }
        else {
            std::lock_guard<std::mutex> lock(instance_mutex_);
            mkdir_helper(path, mode);
        }
    }

    void mkdir_helper(const std::string& path, mode_t mode) {
        // If the directory already exists, return without trying to make it.
        if (access(path.c_str(), F_OK) == 0) {
            return;
        }

        int status = ::mkdir(path.c_str(), mode);
        if (status != 0 /*&& errno != EEXIST*/) {
            throw std::runtime_error("Error creating directory: " + path + "; " + strerror(errno));
        }
    }

private:
    static bool  use_class_mutex_;
    static std::mutex class_mutex_;
    std::mutex instance_mutex_;

    std::string filename_;
    std::ofstream file_;
};

bool FileWriter::use_class_mutex_ = false;
std::mutex FileWriter::class_mutex_;

#endif //BES_FILEWRITER_H
