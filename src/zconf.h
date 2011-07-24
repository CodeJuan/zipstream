/**
* Copyright 2011 Victor Egea Hernando
*
* Zipstream is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, version 3 of the License.
*
* Zipstream is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with Zipstream.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ZCONF_H_
#define ZCONF_H_

#include <iostream>
#include <zlib.h>

// default buffer sizes
#define ZCOBSIZE ( ( 1 << 20 )      ) // 1.0 MB
#define ZCIBSIZE ( ( 1 << 10 ) << 7 ) // 128 KB

namespace zconf {

// pointers and data
typedef char               byte;
typedef byte*              bytep;
typedef const bytep        cbytep;

// integer types
typedef   signed short int  int16;
typedef unsigned short int uint16;
typedef   signed       int  int32;
typedef unsigned       int uint32;
typedef   signed long  int  int64;
typedef unsigned long  int uint64;

}; // namespace zconf

#endif // ZCONF_H_
