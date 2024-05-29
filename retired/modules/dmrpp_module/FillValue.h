//
// Created by James Gallagher on 5/10/22.
//

#ifndef BES_FILLVALUE_H
#define BES_FILLVALUE_H

#include <libdap/BaseType.h>
#include "BESInternalError.h"

namespace dmrpp {

struct FillValue {
    libdap::Type type{libdap::dods_null_c};

    /// @brief hold the value used to fill empty chunks
    union fill_value {
        int8_t int8;
        int16_t int16;
        int32_t int32;
        int64_t int64;

        uint8_t uint8;
        uint16_t uint16;
        uint32_t uint32;
        uint64_t uint64;

        float f;
        double d;
    } value{0};

    FillValue() = default;
    ~FillValue() = default;

#if 0
    /// @{
    /// @name set(<value>)
    /// @brief Set the fill value and type
    void set(int8_t v) { value.int8 = v; type = libdap::dods_int8_c; }
    void set(int16_t v) { value.int16 = v; type = libdap::dods_int16_c; }
    void set(int32_t v) { value.int32 = v; type = libdap::dods_int32_c; }
    void set(int64_t v) { value.int64 = v; type = libdap::dods_int64_c; }

    void set(uint8_t v) { value.uint8 = v; type = libdap::dods_uint8_c; }
    void set(uint16_t v) { value.uint16 = v; type = libdap::dods_uint16_c; }
    void set(uint32_t v) { value.uint32 = v; type = libdap::dods_uint32_c; }
    void set(uint64_t v) { value.uint64 = v; type = libdap::dods_uint64_c; }

    void set(float v) { value.f = v; type = libdap::dods_float32_c; }
    void set(double v) { value.d = v; type = libdap::dods_float64_c; }
    /// @}
#endif

    void set(std::string v, libdap::Type t) {
        type = t;
        switch(type) {
            case libdap::dods_int8_c:
                value.int8 = (int8_t)stoi(v);
                break;

            case libdap::dods_int16_c:
                value.int16 = (int16_t)stoi(v);
                break;

            case libdap::dods_int32_c:
                value.int32 = (int32_t)stol(v);
                break;

            case libdap::dods_int64_c:
                value.int64 = stoll(v);
                break;

            case libdap::dods_byte_c:
            case libdap::dods_uint8_c:
                value.uint8 = (uint8_t)stoi(v);
                break;

            case libdap::dods_uint16_c:
                value.uint16 = (uint16_t)stoi(v);
                break;

            case libdap::dods_uint32_c:
                value.uint32 = (uint32_t)stol(v);
                break;

            case libdap::dods_uint64_c:
                value.uint64 = stoll(v);
                break;

            case libdap::dods_float32_c:
                value.f = stof(v);
                break;

            case libdap::dods_float64_c:
                value.d = stod(v);
                break;

            default:
                throw BESInternalError("Unknown fill value type.", __FILE__, __LINE__);
        }
    }

    libdap::Type get_type() const { return type; }
    
    const char *get_value_ptr() const {
        switch(type) {
            case libdap::dods_int8_c:
                return (const char*)&value.int8;
            case libdap::dods_int16_c:
                return (const char*)&value.int16;
            case libdap::dods_int32_c:
                return (const char*)&value.int32;
            case libdap::dods_int64_c:
                return (const char*)&value.int64;
            case libdap::dods_uint8_c:
                return (const char*)&value.uint8;
            case libdap::dods_uint16_c:
                return (const char*)&value.uint16;
            case libdap::dods_uint32_c:
                return (const char*)&value.uint32;
            case libdap::dods_uint64_c:
                return (const char*)&value.uint64;
                
            case libdap::dods_float32_c:
                return (const char*)&value.f;
            case libdap::dods_float64_c:
                return (const char*)&value.d;
                
            default:
                throw BESInternalError("Unknown fill value type.", __FILE__, __LINE__);
       }
    }

    unsigned int get_value_size() const {
        switch(type) {
            case libdap::dods_int8_c:
                return sizeof(value.int8);
            case libdap::dods_int16_c:
                return sizeof(value.int16);
            case libdap::dods_int32_c:
                return sizeof(value.int32);
            case libdap::dods_int64_c:
                return sizeof(value.int64);
            case libdap::dods_uint8_c:
                return sizeof(value.uint8);
            case libdap::dods_uint16_c:
                return sizeof(value.uint16);
            case libdap::dods_uint32_c:
                return sizeof(value.uint32);
            case libdap::dods_uint64_c:
                return sizeof(value.uint64);

            case libdap::dods_float32_c:
                return sizeof(value.f);
            case libdap::dods_float64_c:
                return sizeof(value.d);

            default:
                throw BESInternalError("Unknown fill value type.", __FILE__, __LINE__);
        }
    }
};

} // dmrpp

#endif //BES_FILLVALUE_H
