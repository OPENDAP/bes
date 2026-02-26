//
// Created by ndp on 7/8/22.
//

#ifndef BES_FONCNAMES_H
#define BES_FONCNAMES_H

#define FONC_TEMP_DIR "/tmp"
#define FONC_TEMP_DIR_KEY "FONc.Tempdir"

// I think this should be true always. I'm leaving it in for now
// but the code does not use the key. Maybe this will be used when
// there is more comprehensive support for DAP4. jhrg 11/30/15
#define FONC_BYTE_TO_SHORT true
#define FONC_BYTE_TO_SHORT_KEY "FONc.ByteToShort"

#define FONC_USE_COMP true
#define FONC_USE_COMP_KEY "FONc.UseCompression"

#define FONC_USE_SHUFFLE false
#define FONC_USE_SHUFFLE_KEY "FONc.UseShuffle"

#define FONC_CHUNK_SIZE 4096
#define FONC_CHUNK_SIZE_KEY "FONc.ChunkSize"

// In fonc.conf.in, the FONC_CLASSIC_MODEL is set to false, which is
// default setting in distribution. So here we change the FONC_CLASSIC_MODEL 
// to false. That means, if FONc.ClassicModel is not set, the fileout netCDF
// will follow netCDF4-enhanced instead of netCDF-4 classic to generate 
// netCDF-4 files.
#define FONC_CLASSIC_MODEL false
#define FONC_CLASSIC_MODEL_KEY "FONc.ClassicModel"

#define FONC_REDUCE_DIM false
#define FONC_REDUCE_DIM_KEY "FONc.ReduceDim"

#define FONC_NO_GLOBAL_ATTRS false
#define FONC_NO_GLOBAL_ATTRS_KEY "FONc.NoGlobalAttrs"

#define FONC_REQUEST_MAX_SIZE_KB 0
#define FONC_REQUEST_MAX_SIZE_KB_KEY "FONc.RequestMaxSizeKB"

#define FONC_NC3_CLASSIC_FORMAT false
#define FONC_NC3_CLASSIC_FORMAT_KEY "FONc.NC3ClassicFormat"

#define FONC_NO_COMPRESSION_FLOAT false
#define FONC_NO_COMPRESSION_FLOAT_KEY "FONc.NoCompressionForFloat"

#define FONC_RETURN_AS_NETCDF3 "netcdf"
#define FONC_RETURN_AS_NETCDF4 "netcdf-4"
#define FONC_NC4_CLASSIC_MODEL "NC4_CLASSIC_MODEL"
#define FONC_NC4_ENHANCED "NC4_ENHANCED"

// May add netCDF-3 CDF-5 in the future.

#endif //BES_FONCNAMES_H
