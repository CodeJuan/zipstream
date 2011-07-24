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

#include "ziparchive.h"

#include <sstream>
#include <fstream>
#include <cstring>
#include <iomanip>
#include <list>
#include <set>

// end of central directory signature
#define ECDSIGN   0x06054b50
// end of central directory file header signature
#define ECDFHSIGN 0x02014b50
// end of local file header signature
#define ELFHSIGN  0x08074b50
// local file header signature
#define LFHSIGN   0x04034b50
// local file header size without extra fields
#define LFHSIZE   30

typedef struct file_info_32{
    zconf::uint16 _version;              // version made by                 2 bytes
    zconf::uint16 _version_needed;       // version needed to extract       2 bytes
    zconf::uint16 _flag;                 // general purpose bit flag        2 bytes
    zconf::uint16 _compression_method;   // compression method              2 bytes
    // dos_date;                         // last mod file date in DOS fmt   4 bytes
    zconf::uint32 _crc;                  // crc-32                          4 bytes
    zconf::uint32 _compressed_size;      // compressed size                 4 bytes
    zconf::uint32 _uncompressed_size;    // uncompressed size               4 bytes
    // size_file_name;                   // filename length                 2 bytes
    // size_file_extra;                  // extra field length              2 bytes
    // size_file_comment;                // file comment length             2 bytes
    zconf::uint16 _disk_num_start;       // disk number start               2 bytes
    zconf::uint16 _internal_fa;          // internal file attributes        2 bytes
    zconf::uint32 _external_fa;          // external file attributes        4 bytes
    zconf::uint32 _relative_offset;      // relative offset to the local    4 bytes
    // ...
    zip_tm        _tmu_date;             // date
    std::string   _file_name;            // file name
    std::string   _file_extra;           // file extra
    std::string   _file_comment;         // file comment
    zconf::uint32 _absolute_offset;      // absolute offset to the data     4 bytes
};

// CODE COMMENT FOOTER (2) <<

class sort_by_name{

public:
	bool operator()( const file_info_32* e1, const file_info_32* e2 ) const{
		return e1->_file_name.compare( e2->_file_name ) < 0;
	}

};

class sort_by_offset{

public:
	bool operator()( const file_info_32* e1, const file_info_32* e2 ) const{
		return e1->_relative_offset < e2->_relative_offset;
	}

};

typedef struct ziparchive::core{
    zconf::uint16 _disk_number;
    zconf::uint16 _cdr_first_disk;
    zconf::uint16 _number_cdr_on_disk;
    zconf::uint16 _total_number_cdr;
    zconf::uint32 _size_cdr;
    zconf::uint32 _offset_cdr_start;
    zconf::uint16 _zip_comment_length;
	std::string   _comment;

	// other properties
	std::string   _error;

	// set of registers sorted by offset
	std::set<file_info_32*, sort_by_offset> _entries_by_offset;
	// set of registers sorted by name
	std::set<file_info_32*, sort_by_name>   _entries_by_name;
	// list of entries
	std::list<zipentry *>                   _open_entries;

	// stream and zip size at opening
	std::fstream  _fstream;
	zconf::uint32 _zipsize;
};

typedef struct zipentry::core{
	// private members
	ziparchive::core *_acore;
	// entry alias
	file_info_32 *_entry;
	// entry zstream
	zstream _zstream;
};

ziparchive::ziparchive( void ){
	_core = new core;
}

ziparchive::ziparchive( const char *path ){
	_core = new core;
	// open the archive
	open( path );
}

ziparchive::~ziparchive( void ){
	close();
	// delete objects
	delete _core;
}

// get entry from archive
zipentry *ziparchive::entry( const std::string &name, zconf::uint32 size, zconf::uint32 flags ){
	if( !( flags & ( zstream::frio | zstream::fwio ) ) ){
		_core->_error = "ziparchive: the entry must be set to be read or written";
		return 0;
	}

	// find the entry
	file_info_32 key; key._file_name = name;
	std::set<file_info_32*, sort_by_name>::iterator entry = _core->_entries_by_name.find( &key );

	if( entry != _core->_entries_by_name.end() ){
		if( (*entry)->_compression_method != 8 && (*entry)->_compression_method != 9 ){
			_core->_error = "ziparchive: compression method not supported";
			return 0;
		}
	}

	if( flags == zstream::frio ){
		if( entry != _core->_entries_by_name.end() ){
			zipentry *zip_entry = new zipentry( *_core, **entry, flags );
			_core->_open_entries.push_back( zip_entry );
			// return entry
			return zip_entry;
		}else{
			return 0;
		}
	}else{
		// check if it's being used
		std::list<zipentry *>::iterator open_entry = _core->_open_entries.begin();
		// go throught the open entries
		while( open_entry != _core->_open_entries.end() ){
			if( !(*open_entry)->_core->_entry->_file_name.compare( name ) ){
				_core->_error = "ziparchive: the entry is already open; you can not modify it!";
				return 0;
			}
			open_entry++; // open next entry
		}

		// remove entry
		if( entry != _core->_entries_by_name.end() ){
			_core->_entries_by_name.erase( *entry );
			_core->_entries_by_offset.erase( *entry );
			delete *entry;
		}

		// create cdr entry
		file_info_32 *cdr_entry = new file_info_32;
		//...

		// find a gap!!
		zconf::uint32 local_size    = size + LFHSIZE + name.length();
		cdr_entry->_relative_offset = find_gap( local_size );
		cdr_entry->_absolute_offset = cdr_entry->_relative_offset + local_size;

		// add entry to sets
		_core->_entries_by_name.insert( cdr_entry );
		_core->_entries_by_offset.insert( cdr_entry );

		// create new entry
		zipentry *zip_entry = new zipentry( *_core, *cdr_entry, flags );
		_core->_open_entries.push_back( zip_entry );

		// return entry
		return zip_entry;
	}
}

zconf::uint32 ziparchive::find_gap( zconf::uint32 size ){
	// check if it's being used
	std::set<file_info_32*, sort_by_offset>::iterator entry = _core->_entries_by_offset.begin();
	zconf::uint32 last_gap_start = 0;
	// go throught the open entries
	while( entry != _core->_entries_by_offset.end() ){
		if( ( (*entry)->_relative_offset - last_gap_start ) >= size ) return last_gap_start;
		last_gap_start = (*entry)->_absolute_offset + (*entry)->_compressed_size;
		entry++; // next entry
	}
	// return last gat start
	return last_gap_start;
}

ziparchive &ziparchive::open( const char *path ){
	// scanning index
	zconf::uint32 sindex = 0;

	// open stream
	_core->_fstream.open( path, std::ios::in | std::ios::out | std::ios::binary | std::ios::app );

	// open archive
	if( _core->_fstream.is_open() ){
		// set the get pointer at the end of the file
		_core->_fstream.seekg( 0, std::ios::end );
		_core->_zipsize = _core->_fstream.tellg();
		// find signature
		sindex = find_signature( ECDSIGN, _core->_zipsize, false );
		// process data
		if( sindex > 0 ){
			sindex += 4;
			// FOUND!! Update scanning index
			_core->_fstream.seekg( sindex, std::ios::beg );
			// read CDR ending record
			_core->_fstream.read( reinterpret_cast<char*>( &_core->_disk_number ),        sizeof( zconf::uint16 ) );
			_core->_fstream.read( reinterpret_cast<char*>( &_core->_cdr_first_disk ),     sizeof( zconf::uint16 ) );
			_core->_fstream.read( reinterpret_cast<char*>( &_core->_number_cdr_on_disk ), sizeof( zconf::uint16 ) );
			_core->_fstream.read( reinterpret_cast<char*>( &_core->_total_number_cdr ),   sizeof( zconf::uint16 ) );
			_core->_fstream.read( reinterpret_cast<char*>( &_core->_size_cdr ),           sizeof( zconf::uint32 ) );
			_core->_fstream.read( reinterpret_cast<char*>( &_core->_offset_cdr_start ),   sizeof( zconf::uint32 ) );
			_core->_fstream.read( reinterpret_cast<char*>( &_core->_zip_comment_length ), sizeof( zconf::uint16 ) );
			// read zip comment
			_core->_comment = std::string().append( _core->_zip_comment_length, ' ' );
			_core->_fstream.read( &_core->_comment[0], _core->_zip_comment_length );
			// read the central directory records
			read_cdr();
		}else{
			_core->_error = "ziparchive: zip file corrupted";
		}
	}else{
		_core->_error = "ziparchive: the file couldn't be open";
	}
	// return object
	return *this;
}

void ziparchive::read_cdr( void ){
	zconf::uint32 sindex, lsindex, word, dos_date;
	zconf::uint16 size_file_name, size_file_extra, size_file_comment;
	// go to the start of the central directory records
	_core->_fstream.seekg( _core->_offset_cdr_start, std::ios::beg );
	// get index
	sindex = _core->_fstream.tellg();
	// process central directories
	for(;;){
		// find the signature
		if( ( sindex = find_signature( ECDFHSIGN, sindex ) ) <= 0 ){
			break; // get out of the loop
		}

		// FOUND!! Update scanning index
		sindex += 4; _core->_fstream.seekg( sindex, std::ios::beg );
		// process the record
		file_info_32 *file_info = new file_info_32;
	    _core->_fstream.read( reinterpret_cast<char*>( &file_info->_version ), sizeof( zconf::uint16 ) );
	    _core->_fstream.read( reinterpret_cast<char*>( &file_info->_version_needed ), sizeof( zconf::uint16 ) );
	    _core->_fstream.read( reinterpret_cast<char*>( &file_info->_flag ), sizeof( zconf::uint16 ) );
	    _core->_fstream.read( reinterpret_cast<char*>( &file_info->_compression_method ), sizeof( zconf::uint16 ) );
	    _core->_fstream.read( reinterpret_cast<char*>( &dos_date ), sizeof( zconf::uint32 ) );
	    _core->_fstream.read( reinterpret_cast<char*>( &file_info->_crc ), sizeof( zconf::uint32 ) );
	    _core->_fstream.read( reinterpret_cast<char*>( &file_info->_compressed_size ), sizeof( zconf::uint32 ) );
	    _core->_fstream.read( reinterpret_cast<char*>( &file_info->_uncompressed_size ), sizeof( zconf::uint32 ) );
	    _core->_fstream.read( reinterpret_cast<char*>( &size_file_name ), sizeof( zconf::uint16 ) );
	    _core->_fstream.read( reinterpret_cast<char*>( &size_file_extra ), sizeof( zconf::uint16 ) );
	    _core->_fstream.read( reinterpret_cast<char*>( &size_file_comment ), sizeof( zconf::uint16 ) );
	    _core->_fstream.read( reinterpret_cast<char*>( &file_info->_disk_num_start ), sizeof( zconf::uint16 ) );
	    _core->_fstream.read( reinterpret_cast<char*>( &file_info->_internal_fa ), sizeof( zconf::uint16 ) );
	    _core->_fstream.read( reinterpret_cast<char*>( &file_info->_external_fa ), sizeof( zconf::uint32 ) );
	    _core->_fstream.read( reinterpret_cast<char*>( &file_info->_relative_offset ), sizeof( zconf::uint32 ) );
	    // convert time stamp
	    file_info->_tmu_date = dosbin2timestamp( dos_date );
		// read file name
	    file_info->_file_name = std::string().append( size_file_name, ' ' );
		_core->_fstream.read( &file_info->_file_name[0], size_file_name );
		// read file extra
		file_info->_file_extra = std::string().append( size_file_extra, ' ' );
		_core->_fstream.read( &file_info->_file_extra[0], size_file_extra );
		// read file comment
		file_info->_file_comment = std::string().append( size_file_comment, ' ' );
		_core->_fstream.read( &file_info->_file_comment[0], size_file_comment );

		// save context
		// lsindex = file_info->_relative_offset;
		sindex = _core->_fstream.tellg();

		// CODE COMMENT FOOTER (1) <<

		// set the absolute data offset
		file_info->_absolute_offset = file_info->_relative_offset + LFHSIZE;
		file_info->_absolute_offset = file_info->_absolute_offset + size_file_name + size_file_extra;

		// add a new entry to the structures
		_core->_entries_by_offset.insert( file_info );
		_core->_entries_by_name.insert( file_info );
	}

	// construct gap structure
	// ...
}

ziparchive &ziparchive::set_comment( const std::string &comment ) const{
	_core->_comment = comment;
}

/*
 * This format stores dates starting from 1980/01/01
 * with a resolution of two seconds.
 */

zip_tm ziparchive::dosbin2timestamp( zconf::uint32 bin ){
	zip_tm timestamp;
	// convert to timestamp
    const zconf::uint32 date = bin >> 16;
    const zconf::uint32 time = bin;
    // ...
    timestamp.tm_year = ((date >> 9) & 0x7f) + 1980;
    timestamp.tm_mon  = ((date >> 5) & 0x0f);
    timestamp.tm_mday = ((date)      & 0x1f);
    // ...
    timestamp.tm_hour = ((time >> 11) & 0x1f);
    timestamp.tm_min  = ((time >>  5) & 0x3f);
    timestamp.tm_sec  = ((time <<  1) & 0x3f);
	// return value
	return timestamp;
}

zconf::uint32 ziparchive::timestamp2dosbin( const zip_tm &timestamp ){
	zconf::uint32 bin;
	// convert to binary
	zconf::uint32 date, time;

    date = ((timestamp.tm_year - 1980) << 9) + ((timestamp.tm_mon) << 5) + timestamp.tm_mday;
    time = (timestamp.tm_hour << 11) + (timestamp.tm_min << 5) + (timestamp.tm_sec >> 1);

    return (( date << 16 ) | time );

	// return value
	return bin;
}

std::string ziparchive::timestamp2string( const zip_tm &timestamp ){
	std::stringstream sstream;
	// convert timestamp to string
	sstream << timestamp.tm_mday << "-" << timestamp.tm_mon << "-" << timestamp.tm_year;
	sstream << ", " << timestamp.tm_hour << ":" << timestamp.tm_min;
	sstream << ", " << timestamp.tm_sec << " secs";
	// return string of the timestamp
	return sstream.str();
}

zconf::uint32 ziparchive::find_signature( zconf::uint32 sign, zconf::uint32 sindex, bool forewards ){
	// scanning buffer size
	const zconf::uint32 SBUFFERS = 32; char sbuffer[ SBUFFERS ];

	// get local scanning index
	zconf::int64 lsindex = sindex;
	// once we have the offset of the file
	if( !forewards ){
		lsindex = ( lsindex - SBUFFERS > 0 ) ? lsindex - SBUFFERS : 0;
	}

	// scan for the signature
	for( ;; ){
		// check boundaries
		if( forewards && ( lsindex >= _core->_zipsize ) ){
			return 0;
		}else if( !forewards && ( lsindex < 0 ) ){
			return 0;
		}
		// calculate chunk size
		zconf::uint32 chunk_size = SBUFFERS;
		if( lsindex + SBUFFERS >= _core->_zipsize )
			chunk_size = _core->_zipsize - lsindex;
		// read chunk
		_core->_fstream.seekg( lsindex, std::ios::beg );
		_core->_fstream.read( sbuffer, chunk_size );
		// find the signature
		for( zconf::uint32 idx = 0; idx < chunk_size - 3; idx++ ){
			// extract current word
			zconf::uint32 word = *( (zconf::uint32 *)( sbuffer + idx ) );
			// make comparison
			if( word == sign ) return lsindex + idx;
		}
		// update index
		if( forewards ){
			lsindex = lsindex + SBUFFERS - 3;
		}else{
			lsindex = lsindex - SBUFFERS + 3;
		}
	}
}

const std::string &ziparchive::error( void ) const{
	return _core->_error;
}

const std::string &ziparchive::comment( void ) const{
	return _core->_comment;
}

// get the full list of entries
std::vector<std::string> ziparchive::entries( void ){
	std::vector<std::string> entries;
	// copy from internal set to vector
	std::set<file_info_32*, sort_by_offset>::iterator first  = _core->_entries_by_name.begin();
	std::set<file_info_32*, sort_by_offset>::iterator last   = _core->_entries_by_name.end();
	// copy the name of the entry
	while ( first != last ) entries.push_back( ( *first++ )->_file_name );
	// return the vector
	return entries;
}

ziparchive &ziparchive::close( void ){
	// close buffer
	_core->_fstream.close();
	// return object
	return *this;
}

bool ziparchive::is_open( void ) const{
	return _core->_fstream.is_open();
}

zipentry::zipentry(){
	// it shouldn't be used!!!
}

zipentry::zipentry( ziparchive::core &acore, file_info_32 &entry, zconf::uint32 flags ){
	_core = new core;

	// assign values
	_core->_acore = &acore;
	_core->_entry = &entry;

	if( _core->_entry->_compressed_size ){
		// open stream
		_core->_zstream.open( _core->_acore->_fstream,
			_core->_entry->_compressed_size, _core->_entry->_uncompressed_size,
			_core->_entry->_absolute_offset, flags | zstream::fzip );
	}
}

zipentry &zipentry::read( zconf::cbytep data, zconf::uint64 nbytes ){
	_core->_zstream.read( data, nbytes );
}

zipentry &zipentry::write( zconf::cbytep data, zconf::uint64 nbytes ){
	_core->_zstream.write( data, nbytes );
}

bool zipentry::eof( void ) const{
	return _core->_zstream.eof();
}

zconf::uint32 zipentry::flags( void ) const{
	return _core->_zstream.flags();
}

zconf::uint64 zipentry::gcount( void ) const{
	return _core->_zstream.gcount();
}

zconf::uint64 zipentry::tcount( void ) const{
	return _core->_zstream.tcount();
}

zconf::uint64 zipentry::zoffset( void ) const{
	return _core->_zstream.zoffset();
}

const std::string &zipentry::error( void ) const{
	return _core->_zstream.error();
}

zipentry::~zipentry(){
	delete _core;
}

void zipentry::close(  void ){
	_core->_zstream.close();
	// remove entry from open list
	_core->_acore->_open_entries.remove( this );
	// delete object
	delete this;
}

bool zipentry::is_open( void ) const{
	return ( _core->_entry != 0 && _core->_acore != 0 );
}

zip_tm zipentry::timestamp( void ) const{
	return _core->_entry->_tmu_date;
}

std::string zipentry::name( void ) const{
	return _core->_entry->_file_name;
}

std::string zipentry::comment( void ) const{
	return _core->_entry->_file_comment;
}

zconf::uint32 zipentry::compressed_size( void ) const{
	return _core->_entry->_compressed_size;
}

zconf::uint32 zipentry::uncompressed_size( void ) const{
	return _core->_entry->_uncompressed_size;
}

/* CODE COMMENT FOOTER (1) >> READ LOCAL FILE HEADER: not necessary?
	// go to the local header
	_core->_fstream.seekg( lsindex, std::ios::beg );
	_core->_fstream.read( reinterpret_cast<char*>( &word ), sizeof( zconf::uint32 ) );
	// check signature
	if( word != LFHSIGN ){
		_core->_error = "ziparchive: a local file header signature is incorrect";
		close(); return;
	}
	// process the record
	local_file_info_32 local_file_info;
	_core->_fstream.read( reinterpret_cast<char*>( &local_file_info._version_needed ), sizeof( zconf::uint16 ) );
	_core->_fstream.read( reinterpret_cast<char*>( &local_file_info._flag ), sizeof( zconf::uint16 ) );
	_core->_fstream.read( reinterpret_cast<char*>( &local_file_info._compression_method ), sizeof( zconf::uint16 ) );
	_core->_fstream.read( reinterpret_cast<char*>( &dos_date ), sizeof( zconf::uint32 ) );
	_core->_fstream.read( reinterpret_cast<char*>( &local_file_info._crc ), sizeof( zconf::uint32 ) );
	_core->_fstream.read( reinterpret_cast<char*>( &local_file_info._compressed_size ), sizeof( zconf::uint32 ) );
	_core->_fstream.read( reinterpret_cast<char*>( &local_file_info._uncompressed_size ), sizeof( zconf::uint32 ) );
	_core->_fstream.read( reinterpret_cast<char*>( &size_file_name ), sizeof( zconf::uint16 ) );
	_core->_fstream.read( reinterpret_cast<char*>( &size_file_extra ), sizeof( zconf::uint16 ) );
	// convert timestamp
	local_file_info._tmu_date = dosbin2timestamp( dos_date );
	// read file name
	local_file_info._file_name = std::string().append( size_file_name, ' ' );
	_core->_fstream.read( &local_file_info._file_name[0], size_file_name );
	// read file extra
	local_file_info._file_extra = std::string().append( size_file_extra, ' ' );
	_core->_fstream.read( &local_file_info._file_extra[0], size_file_extra );
	// get index
	lsindex = _core->_fstream.tellg();

	// 0x08 flag: {crc, compressed data and uncompressed data} at the end of the main record
	if( local_file_info._flag & 0x08 ){
		// go to the end of the data
		if( !( lsindex = find_signature( LFHSIGN, lsindex ) ) ){
			_core->_error = "ziparchive: a local file header signature was not found where flags & 0x08";
			close(); return;
		}
		// FOUND!! Update scanning index
		lsindex += 4; _core->_fstream.seekg( lsindex, std::ios::beg );
		// read the critical data
		_core->_fstream.read( reinterpret_cast<char*>( &local_file_info._crc ), sizeof( zconf::uint32 ) );
		_core->_fstream.read( reinterpret_cast<char*>( &local_file_info._compressed_size ), sizeof( zconf::uint32 ) );
		_core->_fstream.read( reinterpret_cast<char*>( &local_file_info._uncompressed_size ), sizeof( zconf::uint32 ) );
	}

	// combine setting of the entry
	// ...
*/

/* CODE COMMENT FOOTER (2) >> READ LOCAL FILE HEADER: not necessary?
	typedef struct local_file_info_32{
		zconf::uint16 _version_needed;       // version needed to extract       2 bytes
		zconf::uint16 _flag;                 // general purpose bit flag        2 bytes
		zconf::uint16 _compression_method;   // compression method              2 bytes
		// dos_date;                         // last mod file date in DOS fmt   4 bytes
		zconf::uint32 _crc;                  // crc-32                          4 bytes
		zconf::uint32 _compressed_size;      // compressed size                 4 bytes
		zconf::uint32 _uncompressed_size;    // uncompressed size               4 bytes
		// size_file_name;                   // filename length                 2 bytes
		// size_file_extra;                  // extra field length              2 bytes
		// ...
		zip_tm        _tmu_date;             // date
		std::string   _file_name;            // file name
		std::string   _file_extra;           // file extra
	};
*/
