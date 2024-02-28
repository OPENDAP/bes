
// Created by Kent Yang on 02/27/2024

#ifndef HAVE_FLOAT_BYTESWAP_H
#define HAVE_FLOAT_BYTESWAP_H
#include <cstdint>

void swap_float32(char* buf, int64_t num);
void swap_float64(char* buf, int64_t num);
#endif
