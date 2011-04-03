#ifndef ZSTREAM_H_
#define ZSTREAM_H_

#include "zconf.h"

class zstream{

public:
	// default constructor
	zstream( void );
	// constructor 1
	zstream( zconf::bytep data, zconf::uint32 size,
		zconf::uint32 flags = frio, zconf::int32 level = Z_DEFAULT_COMPRESSION );
	// constructor 2
	zstream( std::iostream &ios, zconf::uint64 offset = 0,
		zconf::uint32 flags = frio, zconf::int32 level = Z_DEFAULT_COMPRESSION );

public:
	// open from buffer
	zstream &open( zconf::bytep data, zconf::uint32 size,
		zconf::uint32 flags = frio, zconf::int32 level = Z_DEFAULT_COMPRESSION );
	// open from iostream
	zstream &open( std::iostream &ios, zconf::uint64 offset = 0,
		zconf::uint32 flags = frio, zconf::int32 level = Z_DEFAULT_COMPRESSION );
	// close stream if necessary
	zstream &close( void );
	// tell us if buffer is open
	bool is_open( void ) const;
	// read n bytes and allocate them on data
	zstream &read( zconf::cbytep data, zconf::uint64 nbytes );
	// write n bytes on data
	zstream &write( zconf::cbytep data, zconf::uint64 nbytes );
	// number of bytes treated since the opening
	zconf::uint64 tcount( void ) const;
	// number of bytes treated in the last operation
	zconf::uint64 gcount( void ) const;
	// get active flags
	zconf::uint32 flags( void ) const;
	// end of zstream input
	bool eof( void ) const;
	// flash remaining data to write
	zstream &flush( void );
	// get error string
	const std::string &error( void ) const;
	// set buffer sizes
	zstream &setbs( zconf::uint64 ibs = ZCOBSIZE, zconf::uint64 obs = ZCIBSIZE );

private:
	// number of bytes read in the last operation
	// & bytes of compression treated
	zconf::uint64 _gcount, _tcount, _zoffset;
	// zstream buffers
	zconf::bytep _obuffer, _ibuffer;
	// zstream buffer size
	zconf::uint64 _ozsize, _izsize;
	// remaining data offset
	zconf::uint64 _roffset, _rndata;
	// iostream class pointer
	std::iostream *_ios;
	// active flags
	zconf::uint32 _flags;
	// size of the buffer
	zconf::uint64 _size;
	// stream data pointer
	zconf::bytep _data;
	// error string
	std::string _error;
	// zlib z_stream
	z_stream _zstream;

private:
	// init stream
	void inits( zconf::int32 level );
	// seek to offset
	void seekoffset( void );
public:
	// class flags
	static const zconf::uint32 frio = 0x01; // read
	static const zconf::uint32 fwio = 0x02; // write
	static const zconf::uint32 feof = 0x04; // end of buffer
	static const zconf::uint32 ferr = 0x08; // error happened

};

#endif //ZSTREAM_H_
