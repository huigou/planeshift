/* this tries to emulate a stdint.h */

#ifndef __PSSTDINT_H__
#define __PSSTDINT_H__

#include "cstypes.h"

// Remove *_t definitions to be consistent with CS types

// This "should" actually be true for all c++2011 implementation. Additional versions need to be added if other OS/tool chains break on the redefinition of int8_t
// We do not print this typedef for msvc 2015.
#ifdef _MSC_VER
#if (_MSC_VER < 1900)
typedef int8	int8_t;
#endif
#else
typedef int8	int8_t;
#endif
typedef uint8	uint8_t;
typedef int16	int16_t;
typedef uint16	uint16_t;
typedef int32	int32_t;
typedef uint32	uint32_t;
typedef int64  int64_t;
typedef uint64 uint64_t;

#endif
