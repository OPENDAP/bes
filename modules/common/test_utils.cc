//
// Created by James Gallagher on 3/6/25.
//

// This is not used by the little library here, yet. I made this so I would
// not need to remember to do it later. jhrg 3/6/25

#include <string>
#include <sys/stat.h>

static long get_file_size(const std::string &filename) {
    struct stat st{};
    if (stat(filename.c_str(), &st) == 0) {
        return st.st_size;
    }
    return -1; // Indicate error (file not found, etc.)
}

class file_wrapper {
    std::string d_filename;
public:
    explicit file_wrapper(const std::string &filename) : d_filename(filename) {}
    ~file_wrapper() { remove(d_filename.c_str()); }
};
