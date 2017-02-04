#error This file is now obsolete

#if defined(FF_TYPES_H__) && defined(FF_CHECK_SIZES)

if (SIZE_FLOAT32 != sizeof(float32))
	fprintf(stderr, "Type size mismatch for float32: expected %d, is %d\n",
	       (int)SIZE_FLOAT32, (int)sizeof(float32));

if (SIZE_FLOAT64 != sizeof(float64))
	fprintf(stderr, "Type size mismatch for float64: expected %d, is %d\n",
	       (int)SIZE_FLOAT64, (int)sizeof(float64));

if (SIZE_INT8    != sizeof(int8))
	fprintf(stderr, "Type size mismatch for int8: expected %d, is %d\n",
	       (int)SIZE_INT8, (int)sizeof(int8));

if (SIZE_UINT8   != sizeof(uint8))
	fprintf(stderr, "Type size mismatch for uint8: expected %d, is %d\n",
	       (int)SIZE_UINT8, (int)sizeof(uint8));

if (SIZE_INT16   != sizeof(int16))
	fprintf(stderr, "Type size mismatch for int16: expected %d, is %d\n",
	       (int)SIZE_INT16, (int)sizeof(int16));

if (SIZE_UINT16  != sizeof(uint16))
	fprintf(stderr, "Type size mismatch for uint16: expected %d, is %d\n",
	       (int)SIZE_UINT16, (int)sizeof(uint16));

if (SIZE_INT32   != sizeof(int32))
	fprintf(stderr, "Type size mismatch for int32: expected %d, is %d\n",
	       (int)SIZE_UINT32, (int)sizeof(int32));

if (SIZE_UINT32  != sizeof(uint32))
	fprintf(stderr, "Type size mismatch for uint32: expected %d, is %d\n",
	       (int)SIZE_UINT32, (int)sizeof(uint32));

#ifdef LONGS_ARE_64

if (SIZE_INT64   != sizeof(int64))
	fprintf(stderr, "Type size mismatch for int64: expected %d, is %d\n",
	       (int)SIZE_UINT64, (int)sizeof(int64));

if (SIZE_UINT64  != sizeof(uint64))
	fprintf(stderr, "Type size mismatch for uint64: expected %d, is %d\n",
	       (int)SIZE_UINT64, (int)sizeof(uint64));

#endif /* LONGS_ARE_64 */

#endif /* defined(FF_TYPES_H__) && defined(FF_CHECK_SIZES) */

#ifndef FF_TYPES_H__
#define FF_TYPES_H__

#endif /* FF_TYPES_H__ */

