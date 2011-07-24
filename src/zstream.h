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

#ifndef ZSTREAM_H_
#define ZSTREAM_H_

#include "zconf.h"

/**
 * @author Víctor Egea Hernando, egea.hernando@gmail.com
 *
 * zstream allows you to compress & uncompress a stream using the
 * DEFLATE algorithm through zlibm, but in a write & read
 * the number of bytes you want fashion, so you not depend on
 * how you should feed zlib to get the chunk of data you want, the
 * data its required to zlib, stored and managed automatically
 * <br /><br />
 * zstream doen't offer you random access to the stream, this
 * must be sequential, so it doesn't offer you 'seekp' or 'seekp'
 * <br /><br />
 * raw data compression is given with the 'fzip' flag up
 * <br /><br />
 * there are alternative in the boost libreary for example, but the
 * idea of this system is to make something simple and not depend on
 * any other external libraries but zlib.
 */
class zstream{

public:
	// default constructor
	zstream( void );
	// constructor 1
	zstream( zconf::bytep data,
		zconf::uint32 csize,
		zconf::uint32 usize,
		zconf::uint32 flags = frio,
		zconf::int32 level = Z_DEFAULT_COMPRESSION );
	// constructor 2
	zstream( std::iostream &ios,
		zconf::uint32 csize,
		zconf::uint32 usize,
		zconf::uint64 offset = 0,
		zconf::uint32 flags = frio,
		zconf::int32 level = Z_DEFAULT_COMPRESSION );
	// destructor
	virtual ~zstream( void );

public:
	// open from buffer
	zstream &open( zconf::bytep data,
		zconf::uint32 csize,
		zconf::uint32 usize,
		zconf::uint32 flags = frio,
		zconf::int32 level = Z_DEFAULT_COMPRESSION );
	// open from iostream
	zstream &open( std::iostream &ios,
		zconf::uint32 csize,
		zconf::uint32 usize,
		zconf::uint64 offset = 0,
		zconf::uint32 flags = frio,
		zconf::int32 level = Z_DEFAULT_COMPRESSION );
	// set buffer sizes
	zstream &setbs( zconf::uint64 ibs = ZCOBSIZE,
		zconf::uint64 obs = ZCIBSIZE );
	// close stream if necessary
	zstream &close( void );
	// tell us if buffer is open
	bool is_open( void ) const;

public:
	// read n bytes and allocate them on data
	zstream &read( zconf::cbytep data, zconf::uint64 nbytes );
	// write n bytes on data
	zstream &write( zconf::cbytep data, zconf::uint64 nbytes );
	// flush remaining data
	zstream &flush( void );

public:
	// get error string
	const std::string &error( void ) const;
	// number of bytes treated since the opening
	zconf::uint64 tcount( void ) const;
	// data buffer offset
	zconf::uint64 zoffset( void ) const;
	// number of bytes treated in the last operation
	zconf::uint64 gcount( void ) const;
	// get active flags
	zconf::uint32 flags( void ) const;
	// end of zstream input
	bool eof( void ) const;

private:
	// class core structure declaration
	typedef struct core;
	// internal core structure
	core *_core;

private:
	// init stream
	void inits( zconf::int32 level );
	// seek to offset
	void seekoffset( void );

public:
	// class flags
	static const zconf::uint32 frio    = 0x01; // read
	static const zconf::uint32 fwio    = 0x02; // write
	static const zconf::uint32 feof    = 0x04; // end of buffer
	static const zconf::uint32 ferr    = 0x08; // error happened
	static const zconf::uint32 fzip    = 0x10; // zip entry stream

};

#endif //ZSTREAM_H_
