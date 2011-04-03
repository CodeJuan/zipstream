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

zstream::zstream( void ){
	// set values to zero
	_flags = 0; _data = 0; _ios = 0;
	_obuffer = 0; _ozsize = ZCOBSIZE;
	_ibuffer = 0; _izsize = ZCIBSIZE;
}

zstream::zstream( zconf::bytep data, zconf::uint32 size, zconf::uint32 flags, zconf::int32 level ){
	// set values to zero
	_flags = 0; _data = 0; _ios = 0;
	_obuffer = 0; _ozsize = ZCOBSIZE;
	_ibuffer = 0; _izsize = ZCIBSIZE;

	// open buffer
	open( data, size, flags );
}

zstream::zstream( std::iostream &ios, zconf::uint64 offset, zconf::uint32 flags, zconf::int32 level ){
	// set values to zero
	_flags = 0; _data = 0; _ios = 0;
	_obuffer = 0; _ozsize = ZCOBSIZE;
	_ibuffer = 0; _izsize = ZCIBSIZE;

	// open buffer
	open( ios, offset, flags );
}

// initialize opening
void zstream::inits( zconf::int32 level ){
	// check input sources
	if( _ios != 0 || _data != 0 ){
		_error = "zstream: is already open";
		close(); _flags |= ferr;
	}else{
		// check flags
		if( ( _flags & fwio ) && ( _flags & frio ) ){
			_error = "zstream: can not read and write at the same time";
			close(); _flags |= ferr;
		}else if( _flags & ( ferr | feof ) ){
			_error = "zstream: you can not set the 'err' or 'eof' flag";
			close(); _flags |= ferr;
		}
	}

	// allocate zstream state
	_zstream.zalloc = Z_NULL;
	_zstream.zfree = Z_NULL;
	_zstream.opaque = Z_NULL;

	// prepare zstream
	if( _flags & frio ){
		// allocate inflate state
		_zstream.avail_in = 0;
		_zstream.next_in = Z_NULL;
		// init inflate
		if ( inflateInit( &_zstream ) != Z_OK ){
			_error = "zstream: zlib error";
			close(); _flags |= ferr;
		}
		// create buffers
		if( _ios != 0 ) {
			_ibuffer = new zconf::byte[ _izsize ];
		}
	}else{
		// allocate deflate state
		if ( deflateInit( &_zstream, level ) != Z_OK ){
			_error = "zstream: zlib error";
			close(); _flags |= ferr;
		}
	}
	// create buffers
	_obuffer = new zconf::byte[ _ozsize ];

	// set offsets and other counters
	_zoffset = _roffset = _rndata = 0;
	// bytes treated
	_gcount = _tcount = 0;
}

zstream &zstream::open( zconf::bytep data, zconf::uint32 size,
		zconf::uint32 flags, zconf::int32 level ){
	// set values
	_size = size; _flags = flags; _zoffset = 0;
	// check opening
	inits( level ); _data = data;
	// return reference
	return *this;
}

zstream &zstream::open( std::iostream &ios, zconf::uint64 offset,
		zconf::uint32 flags, zconf::int32 level ){
	// set values
	_flags = flags; _zoffset = offset;
	// check opening
	inits( level ); _ios = &ios;
	// check offset
	if( !( _flags & ferr ) ){
		// seek file
		_ios->seekg( std::ios_base::beg, ( std::_Ios_Seekdir ) offset );
		// check it out
		if( _ios->rdstate() & ( std::iostream::failbit | std::iostream::badbit ) ){
			_error = "zstream: wasn't able to apply the offset";
			close(); _flags |= ferr;
		}
	}
	// return reference
	return *this;
}

zstream &zstream::close( void ){
	// flush if necessary
	if( _flags & fwio ) flush();
	// end zstream states
	if( _flags & frio ){
		inflateEnd( &_zstream );
	}else{
		deflateEnd( &_zstream );
	}
	// reset pointers
	_data = 0; _ios = 0;
	// delete allocated buffers
	if( _obuffer == 0 ){
		delete _obuffer; _obuffer = 0;
	}
}

bool zstream::is_open( void ) const{
	return ( _ios != 0 || _data != 0 );
}

zconf::uint64 zstream::gcount( void ) const{
	return _gcount;
}

zconf::uint64 zstream::tcount( void ) const{
	return _tcount;
}

zconf::uint32 zstream::flags( void ) const{
	return _flags;
}

bool zstream::eof( void ) const{
	return ( _flags & feof );
}

const std::string &zstream::error( void ) const{
	return _error;
}

void zstream::seekoffset( void ){
	if( _ios != 0 ){
		// check end of buffer
		if( _ios->eof() ) {
			_error = "zstream: has reached iostream eof without getting zstream eof";
			_flags |= ferr | feof; return;
		}
		// seek file
		_ios->seekg( std::ios_base::beg, (std::_Ios_Seekdir) _zoffset );
		// check it out
		if( _ios->rdstate() & ( std::iostream::failbit | std::iostream::badbit ) ){
			_error = "zstream: wasn't able to apply the offset";
			close(); _flags |= ferr; return;
		}
	}
}

zstream &zstream::read( zconf::cbytep data, zconf::uint64 nbytes ){
	// reset _gcount
	_gcount = 0;

	// check end of file
	if( _flags & feof || ( _ios == 0 && _data == 0 ) ) return *this;
	// check mode
	if( _flags & fwio ){
		_error = "zstream: is set to read into the buffer";
		_flags |= ferr; return *this;
	}

	// go to the actual offset ( for multithreading over the same iostream )
	seekoffset(); if( _flags & ferr ) return *this;

	// copy remaining data
	if( _rndata ){
		// calculate size to copy
		zconf::uint64 size = ( nbytes < _rndata ) ? nbytes : _rndata;
		// copy to data
		std::memcpy( data, _obuffer + _roffset - _rndata, size );
		// increase gcount and decrease remaining data offset
		_tcount += size; _gcount += size;
		_rndata -= size; _zoffset += size;

		// return object reference if it's done
		if( _gcount == nbytes ) {
			// check eof
			if( _zoffset >= _size  ) _flags |= feof;
			// return object reference
			return *this;  // SUCCESS
		}
	}

	// inflate stream
	do{
		// check if there's remaining data to inflate in the zstream
		if( _zstream.avail_out > 0 ){
			zconf::uint64 isize;
			// prepare nput buffer
			if( _ios != 0 ){
				// read input buffer
				_ios->read( _ibuffer, _izsize );
				// check end of file
				if( _ios->eof() ) _flags |= feof;
				// get the number of bytes read
				isize = _ios->gcount();
			}else{
				// calculate input buffer size
				if( _zoffset + _izsize > _size ) {
					isize = _size - _zoffset;
					// set end of file
					_flags |= feof;
				}else{
					isize = _izsize;
				}
				// get pointer to buffer
				_ibuffer = _data + _zoffset;
			}

			// increase zoffset
			_zoffset += isize;

			// set zstream
			_zstream.avail_in  = isize;
			_zstream.next_in   = reinterpret_cast<Bytef*>( _ibuffer );
			_zstream.avail_out = _ozsize;
			_zstream.next_out  = reinterpret_cast<Bytef*>( _obuffer );
		} // if( _zstream.avail_out > 0 )

		// inflate buffer
		int ret = inflate( &_zstream, Z_NO_FLUSH );
		// check for errors 1
		if( ret == Z_STREAM_ERROR ){
			_error = "zstream: zlib internal error";
			_flags |= ferr; return *this;
		}
		// check for errors 2
		switch ( ret ) {
			case Z_NEED_DICT:{
				_error = "zstream: the entry requires zlib dictionary";
				_flags |= ferr; return *this;
			}case Z_DATA_ERROR:{
				_error = "zstream: zlib data error";
				_flags |= ferr; return *this;
			}case Z_MEM_ERROR:{
				_error = "zstream: zlib memory error";
				_flags |= ferr; return *this;
			}
		}

		// get obtained data size
		zconf::uint64 have = _ozsize - _zstream.avail_out;

		// copy obtained data
		if( _gcount + have > nbytes ){
			zconf::uint64 size = nbytes - _gcount;
			// copy the final data
			memcpy( data + _gcount, _obuffer, size );
			// check eof
			if( _zoffset >= _size  ) _flags |= feof;
			// refresh remaining data values
			_rndata = have - size; _roffset = have;
			// refresh gcount
			_gcount += size; _tcount += size;
			// it's done
			return *this; // SUCCESS
		}else{
			// copy data
			memcpy( data + _gcount, _obuffer, have );
			// refresh gcount
			_gcount += have; _tcount += have;
		}
	}while( _zstream.avail_out == 0 );
}

zstream &zstream::write( const zconf::cbytep data, zconf::uint64 nbytes ){
	// reset _gcount
	_gcount = 0;

	// check end of file
	if( _flags & feof || ( _ios == 0 && _data == 0 ) ) return *this;
	// check mode
	if( _flags & frio ){
		_error = "zstream: is set to write into the buffer";
		_flags |= ferr; return *this;
	}

	// go to the actual offset ( for multithreading over the same iostream )
	seekoffset(); if( _flags & ferr ) return *this;

	// zstream input
	_zstream.avail_in = nbytes;
	_zstream.next_in  = reinterpret_cast<Bytef*>( data );

	do{
		// zstream output
		_zstream.avail_out = _ozsize;
		_zstream.next_out  = reinterpret_cast<Bytef*>( _obuffer );
		// deflate stream
		if( deflate( &_zstream, Z_NO_FLUSH ) == Z_STREAM_ERROR ){
			_error = "zstream: zlib error";
			_flags |= ferr; return *this;
		}
		// write the result
		zconf::uint64 have = _ozsize -_zstream.avail_out;
		if( _ios != 0 ){
			_ios->write( reinterpret_cast<zconf::bytep>( _obuffer ), have );
		}else{
			// check boundaries
			if( _zoffset + have >= _size  ){
				_error = "zstream: overflow of data buffer";
				_flags |= ferr; return *this;
			}
			// memory copy
			std::memcpy( _data + _zoffset, _obuffer, have );
		}
		// number of bytes written
		_gcount += nbytes; _tcount += nbytes; _zoffset += have;
	}while( _zstream.avail_out == 0 );

	// return object
	return *this;
}

zstream &zstream::setbs( zconf::uint64 ibs, zconf::uint64 obs ){
	if( _ios != 0 || _data != 0 ){
		_error = "zstream: is already open";
		close(); _flags |= ferr;
	}
	// set buffer sizes
	_izsize = ibs; _ozsize = obs;
}

zstream &zstream::flush( void ){
	// reset _gcount
	_gcount = 0;

	// check end of file
	if( _flags & feof || ( _ios == 0 && _data == 0 ) ) return *this;
	// check eof
	if( _flags & feof ) return *this;
	// check mode
	if( _flags & frio ){
		_error = "zstream: is set to write into the buffer";
		_flags |= ferr; return *this;
	}

	// go to the actual offset ( for multithreading over the same iostream )
	seekoffset(); if( _flags & ferr ) return *this;

	// zstream input
	_zstream.avail_in = 0;
	_zstream.next_in  = 0;

	// flush zstream contents
	do{
		// zstream output
		_zstream.avail_out = _ozsize;
		_zstream.next_out  = reinterpret_cast<Bytef*>( _obuffer );
		// deflate the rest of the stream
		if( deflate( &_zstream, Z_FINISH ) == Z_STREAM_ERROR ){
			_error = "zstream: zlib error";
			_flags |= ferr; return *this;
		}
		// write the result
		zconf::uint64 have = _ozsize -_zstream.avail_out;
		if( _ios != 0 ){
			_ios->write( reinterpret_cast<zconf::bytep>( _obuffer ), have );
		}else{
			// check boundaries
			if( _zoffset + have >= _size  ){
				_error = "zstream: overflow of data buffer";
				_flags |= ferr; return *this;
			}
			// memory copy
			std::memcpy( _data + _zoffset, _obuffer, have );
		}
		// number of bytes written
		_zoffset += have;
	} while( _zstream.avail_out == 0 );

	// return reference
	return *this;
}
