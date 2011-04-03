#ifndef ZCONF_H_
#define ZCONF_H_

#include <iostream>
#include <zlib/zlib.h>

// default buffer sizes
#define ZCOBSIZE ( ( 1 << 20 ) >> 1 ) // 0.5 MB
#define ZCIBSIZE ( ( 1 << 10 ) << 2 ) // 2   KB

namespace zconf {

// pointers and data
typedef char              byte;
typedef byte*             bytep;
typedef const bytep       cbytep;

// integer types
typedef   signed      int  int32;
typedef unsigned      int uint32;
typedef unsigned long int uint64;

}; // namespace zconf

#endif // ZCONF_H_
