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

#include "zstream.h"

#include <cstring>

typedef struct zstream::core {
	// number of bytes read in the last operation
	// & bytes of compression treated ( initial & current )
	zconf::uint64 _gcount, _tcount, _izoffset, _zoffset;
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
};

zstream::zstream( void ){
	// init core structure
	_core = new core;
	// set values to zero
	_core->_flags = 0; _core->_data = 0; _core->_ios = 0;
}

zstream::~zstream( void ){
	close(); delete _core;
}

zstream::zstream( zconf::bytep data, zconf::uint32 size, zconf::uint32 flags, zconf::int32 level ){
	// init core structure
	_core = new core;
	// set values to zero
	_core->_flags = 0; _core->_data = 0; _core->_ios = 0;

	// open buffer
	open( data, size, flags );
}

zstream::zstream( std::iostream &ios, zconf::uint32 size,
		zconf::uint64 offset, zconf::uint32 flags, zconf::int32 level ){
	// init core structure
	_core = new core;
	// set values to zero
	_core->_flags = 0; _core->_data = 0; _core->_ios = 0;

	// open buffer
	open( ios, size, offset, flags );
}

// initialize opening
void zstream::inits( zconf::int32 level ){
	// check input sources
	if( _core->_ios != 0 || _core->_data != 0 ){
		_core->_error = "zstream: is already open";
		 _core->_flags |= ferr;
	}else{
		// check flags
		if( ( ( _core->_flags & ( fwio | frio ) ) == ( fwio | frio ) ) ){
			_core->_error = "zstream: can not read and write at the same time";
			_core->_flags |= ferr;
		}else if( _core->_flags & ( ferr | feof ) ){
			_core->_error = "zstream: you can not set the 'err' or 'eof' flag";
			_core->_flags |= ferr;
		}
	}

	// init buffers to zero
	_core->_obuffer = 0; _core->_ozsize = ZCOBSIZE;
	_core->_ibuffer = 0; _core->_izsize = ZCIBSIZE;

	// allocate zstream state
	_core->_zstream.zalloc = Z_NULL;
	_core->_zstream.zfree = Z_NULL;
	_core->_zstream.opaque = Z_NULL;

	// prepare zstream
	if( _core->_flags & frio ){
		// allocate inflate state
		_core->_zstream.avail_in = 0;
		_core->_zstream.next_in = Z_NULL;
		// init inflate
		if ( inflateInit( &_core->_zstream ) != Z_OK ){
			_core->_error = "zstream: zlib error";
			 _core->_flags |= ferr;
		}
	}else{
		// allocate deflate state
		if ( deflateInit( &_core->_zstream, level ) != Z_OK ){
			_core->_error = "zstream: zlib error";
			 _core->_flags |= ferr;
		}
	}
	// create buffers
	_core->_ibuffer = new zconf::byte[ _core->_izsize ];
	_core->_obuffer = new zconf::byte[ _core->_ozsize ];

	// set offsets and other counters
	_core->_roffset = _core->_rndata = 0;
	// bytes treated
	_core->_gcount = _core->_tcount = 0;
}

zstream &zstream::open( zconf::bytep data, zconf::uint32 size,
		zconf::uint32 flags, zconf::int32 level ){
	// set values
	_core->_size = size; _core->_flags = flags;
	_core->_izoffset = _core->_zoffset = 0;
	// check opening
	inits( level ); _core->_data = data;
	// return reference
	return *this;
}

zstream &zstream::open( std::iostream &ios, zconf::uint32 size,
		zconf::uint64 offset, zconf::uint32 flags, zconf::int32 level ){
	// set values
	_core->_size = size; _core->_flags = flags;
	_core->_izoffset = _core->_zoffset = offset;
	// check opening
	inits( level ); _core->_ios = &ios;
	// return reference
	return *this;
}

zstream &zstream::close( void ){
	// end zstream states
	if( _core->_flags & fwio ){
		flush();
		deflateEnd( &_core->_zstream );
	}else{
		inflateEnd( &_core->_zstream );
	}
	// delete allocated buffers
	if( _core->_ibuffer != 0 && _core->_ios != 0 ){
		delete _core->_ibuffer;
		_core->_ibuffer = 0;
	}
	if( _core->_obuffer != 0 ){
		// delete allocated buffers
		delete _core->_obuffer;
		_core->_obuffer = 0;
	}
	// reset pointers
	_core->_data = 0; _core->_ios = 0;
}

bool zstream::is_open( void ) const{
	return ( _core->_ios != 0 || _core->_data != 0 );
}

zconf::uint64 zstream::gcount( void ) const{
	return _core->_gcount;
}

zconf::uint64 zstream::tcount( void ) const{
	return _core->_tcount;
}

zconf::uint64 zstream::zoffset( void ) const{
	return _core->_zoffset;
}

zconf::uint32 zstream::flags( void ) const{
	return _core->_flags;
}

bool zstream::eof( void ) const{
	return ( _core->_flags & feof );
}

const std::string &zstream::error( void ) const{
	return _core->_error;
}

void zstream::seekoffset( void ){
	if( _core->_ios != 0 ){
		// check end of buffer
		if( _core->_ios->eof() ) {
			_core->_error = "zstream: has reached iostream eof";
			_core->_flags |= ferr | feof; return;
		}

		// seek file
		if( _core->_ios != 0 ){
			// PUT POINTER
			_core->_ios->seekp( _core->_zoffset, std::ios::beg );
		}else{
			// GET POINTER
			_core->_ios->seekg( _core->_zoffset, std::ios::beg );
		}
		// check it out
		if( _core->_ios->rdstate() & ( std::iostream::failbit | std::iostream::badbit ) ){
			_core->_error = "zstream: wasn't able to apply the offset";
			_core->_flags |= ferr; return;
		}
	}
}

zstream &zstream::read( zconf::cbytep data, zconf::uint64 nbytes ){
	// reset _core->_gcount
	_core->_gcount = 0;

	// check mode
	if( _core->_flags & fwio ){
		_core->_error = "zstream: is set to read into the buffer";
		_core->_flags |= ferr; return *this;
	}
	// check end of file
	if( _core->_flags & feof || ( _core->_ios == 0 && _core->_data == 0 ) ) {
		return *this;
	}

	// copy remaining data
	if( _core->_rndata ){
		zconf::uint64 size;
		// calculate size to copy
		if( nbytes < _core->_rndata ){
			size = nbytes;
		}else{
			size = _core->_rndata;
		}
		// copy to data
		std::memcpy( data, _core->_obuffer + _core->_roffset - _core->_rndata, size );
		// increase gcount and decrease remaining data offset
		_core->_tcount += size; _core->_gcount += size;
		_core->_rndata -= size; _core->_zoffset += size;

		// return object reference if it's done
		if( _core->_gcount == nbytes ) {
			// return object reference
			return *this;  // SUCCESS
		}
	}

	// check eof
	if( _core->_tcount >= _core->_size ){
		_core->_flags |= feof; return *this;
	}

	// go to the actual offset ( for multithreading over the same iostream )
	seekoffset(); if( _core->_flags & ferr ) return *this;

	// inflate stream
	do{
		// check if there's remaining data to inflate in the zstream
		if( _core->_zstream.avail_out > 0 ){
			zconf::uint64 isize;
			// prepare input buffer
			if( _core->_ios != 0 ){
				// check end of file
				if( _core->_ios->eof() ) {
					_core->_flags |= feof; return *this;
				}
				// read input buffer
				_core->_ios->read( _core->_ibuffer, _core->_izsize );
				// get the number of bytes read
				isize = _core->_ios->gcount();
			}else{
				// calculate input buffer size
				if( _core->_zoffset + _core->_izsize > _core->_size ) {
					isize = _core->_size - _core->_zoffset;
					// set end of file
					_core->_flags |= feof;
				}else{
					isize = _core->_izsize;
				}
				// get pointer to buffer
				_core->_ibuffer = _core->_data + _core->_zoffset;
			}

			// increase zoffset
			_core->_zoffset += isize;

			// set zstream
			_core->_zstream.avail_in  = isize;
			_core->_zstream.next_in   = reinterpret_cast<Bytef*>( _core->_ibuffer );
			_core->_zstream.avail_out = _core->_ozsize;
			_core->_zstream.next_out  = reinterpret_cast<Bytef*>( _core->_obuffer );
		} // if( _core->_zstream.avail_out > 0 )

		// inflate buffer
		int ret = inflate( &_core->_zstream, Z_NO_FLUSH );
		// check for errors 1
		if( ret == Z_STREAM_ERROR ){
			_core->_error = "zstream: zlib internal error";
			_core->_flags |= ferr; return *this;
		}
		// check for errors 2
		switch ( ret ) {
			case Z_NEED_DICT:{
				_core->_error = "zstream: the entry requires zlib dictionary";
				_core->_flags |= ferr; return *this;
			}case Z_DATA_ERROR:{
				_core->_error = "zstream: zlib data error";
				_core->_flags |= ferr; return *this;
			}case Z_MEM_ERROR:{
				_core->_error = "zstream: zlib memory error";
				_core->_flags |= ferr; return *this;
			}
		}

		// get obtained data size
		zconf::uint64 have = _core->_ozsize - _core->_zstream.avail_out;

		// copy obtained data
		if( _core->_gcount + have > nbytes ){
			zconf::uint64 size = nbytes - _core->_gcount;
			// copy the final data
			memcpy( data + _core->_gcount, _core->_obuffer, size );
			// check eof
			if( _core->_zoffset - _core->_izoffset >= _core->_size ){
				_core->_flags |= feof;
			}
			// refresh remaining data values
			_core->_rndata = have - size; _core->_roffset = have;
			// refresh gcount
			_core->_gcount += size; _core->_tcount += size;
			// it's done
			return *this; // SUCCESS
		}else{
			// copy data
			memcpy( data + _core->_gcount, _core->_obuffer, have );
			// refresh gcount
			_core->_gcount += have; _core->_tcount += have;
		}
	}while( _core->_zstream.avail_out == 0 );
}

zstream &zstream::write( const zconf::cbytep data, zconf::uint64 nbytes ){
	// reset _core->_gcount
	_core->_gcount = 0;

	// check end of file
	if( _core->_flags & feof || ( _core->_ios == 0 && _core->_data == 0 ) ) {
		return *this;
	}
	// check mode
	if( _core->_flags & frio ){
		_core->_error = "zstream: is set to write into the buffer";
		_core->_flags |= ferr; return *this;
	}

	// go to the actual offset ( for multithreading over the same iostream )
	seekoffset(); if( _core->_flags & ferr ) return *this;

	// zstream input
	_core->_zstream.avail_in = nbytes;
	_core->_zstream.next_in  = reinterpret_cast<Bytef*>( data );

	do{
		// zstream output
		_core->_zstream.avail_out = _core->_ozsize;
		_core->_zstream.next_out  = reinterpret_cast<Bytef*>( _core->_obuffer );
		// deflate stream
		if( deflate( &_core->_zstream, Z_NO_FLUSH ) == Z_STREAM_ERROR ){
			_core->_error = "zstream: zlib error";
			_core->_flags |= ferr; return *this;
		}
		// write the result
		zconf::uint64 have = _core->_ozsize -_core->_zstream.avail_out;
		if( _core->_ios != 0 ){
			_core->_ios->write( reinterpret_cast<zconf::bytep>( _core->_obuffer ), have );
		}else{
			// check boundaries
			if( _core->_zoffset - _core->_izoffset + have >= _core->_size  ){
				_core->_error = "zstream: overflow of data buffer";
				_core->_flags |= ferr; return *this;
			}
			// memory copy
			std::memcpy( _core->_data + _core->_zoffset, _core->_obuffer, have );
		}
		// number of bytes written
		_core->_gcount += nbytes; _core->_tcount += nbytes; _core->_zoffset += have;
	}while( _core->_zstream.avail_out == 0 );

	// return object
	return *this;
}

zstream &zstream::flush( void ){
	// reset _core->_gcount
	_core->_gcount = 0;

	// check end of file
	if( _core->_flags & feof || ( _core->_ios == 0 && _core->_data == 0 ) ) {
		return *this;
	}
	// check mode
	if( _core->_flags & frio ){
		_core->_error = "zstream: is set to write into the buffer";
		_core->_flags |= ferr; return *this;
	}

	// go to the actual offset ( for multithreading over the same iostream )
	seekoffset(); if( _core->_flags & ferr ) return *this;

	// zstream input
	_core->_zstream.avail_in = 0;
	_core->_zstream.next_in  = 0;

	// flush zstream contents
	do{
		// zstream output
		_core->_zstream.avail_out = _core->_ozsize;
		_core->_zstream.next_out  = reinterpret_cast<Bytef*>( _core->_obuffer );
		// deflate the rest of the stream
		if( deflate( &_core->_zstream, Z_FINISH ) == Z_STREAM_ERROR ){
			_core->_error = "zstream: zlib error";
			_core->_flags |= ferr; return *this;
		}
		// write the result
		zconf::uint64 have = _core->_ozsize -_core->_zstream.avail_out;
		if( _core->_ios != 0 ){
			_core->_ios->write( reinterpret_cast<zconf::bytep>( _core->_obuffer ), have );
		}else{
			// check boundaries
			if( _core->_zoffset - _core->_izoffset + have >= _core->_size  ){
				_core->_error = "zstream: overflow of data buffer";
				_core->_flags |= ferr; return *this;
			}
			// memory copy
			std::memcpy( _core->_data + _core->_zoffset, _core->_obuffer, have );
		}
		// number of bytes written
		_core->_zoffset += have;
	} while( _core->_zstream.avail_out == 0 );

	// return reference
	return *this;
}

zstream &zstream::setbs( zconf::uint64 ibs, zconf::uint64 obs ){
	if( _core->_ios != 0 || _core->_data != 0 ){
		_core->_error = "zstream: is already open";
		 _core->_flags |= ferr;
	}
	// set buffer sizes
	_core->_izsize = ibs; _core->_ozsize = obs;
}
