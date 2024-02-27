# Created by KY 2/27/2024
#include "float_byteswap.h"
#include <algorithm>
#include <cstddef>

void swap_float32(char* buf, int64_t num) {

    char *temp_buf = buf;
    while (num--) {
        for (size_t i = 0; i <sizeof(float)/2;i++)
            std::swap(temp_buf[i],temp_buf[sizeof(float)-i-1]);
        temp_buf = temp_buf + sizeof(float);
    }
}

void swap_float64(char* buf, int64_t num) {

    char *temp_buf = buf;
    while (num--) {
        for (size_t i = 0; i <sizeof(double)/2;i++)
            std::swap(temp_buf[i],temp_buf[sizeof(double)-i-1]);
        temp_buf = temp_buf + sizeof(double);
    }
}
