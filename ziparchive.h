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

#ifndef ZIPARCHIVE_H_
#define ZIPARCHIVE_H_

#include "zconf.h"
#include "zstream.h"

#include <vector>

// special types
typedef struct file_info_32;
typedef struct zip_tm;
class zipentry;

class ziparchive {

public:
	// default constructor
	ziparchive( void );
	// constructor 2
	ziparchive( const char *path );
	// destructor
	virtual ~ziparchive( void );

public:
	// get entry from archive
	zipentry *entry( const std::string &name,
		zconf::uint32 size = 0, zconf::uint32 flags = zstream::frio );
	// set zip comment
	ziparchive &set_comment( const std::string &comment ) const;
	// get zip comment
	const std::string &comment( void ) const;
	// get the full list of entries
	std::vector<std::string> entries( void );
	// get error string
	const std::string &error( void ) const;
	// open from iostream
	ziparchive &open( const char *path );
	// defrag archive
	ziparchive &defrag( void );
	// close archive if necessary
	ziparchive &close( void );
	// tell us if archive is open
	bool is_open( void ) const;

public:
	// functions: convert timestamp to string
	static std::string timestamp2string( const zip_tm &timestamp );

private:
	// find signature from the get cursor backwards
	zconf::uint32 find_signature( zconf::uint32 sign, zconf::uint32 sindex, bool forewards = true );
	// convert timestamp to binary
	zconf::uint32 timestamp2dosbin( const zip_tm &timestamp );
	// convert binary to timestamp
	zip_tm dosbin2timestamp( zconf::uint32 bin );
	// find a gap inside the local space
	zconf::uint32 find_gap( zconf::uint32 size );
	// read central directory records
	void read_cdr( void );

public:
	// friend classes
	friend class zipentry;

private:
	// class core structure declaration
	typedef struct core;
	// internal core structure
	core *_core;

};

class zipentry{

public:
	// public destructor
	virtual ~zipentry();

	// public interface
	void close(  void );
	// tell us if archive is open
	bool is_open( void ) const;
	// get timestamp
	zip_tm timestamp( void ) const;
	// name of the entry
	std::string name( void ) const;
	// comment of the entry
	std::string comment( void ) const;
	// get compressed size
	zconf::uint32 compressed_size( void ) const;
	// get uncompressed size
	zconf::uint32 uncompressed_size( void ) const;

	// ZSTREAM INTERFACE

	// end of zstream input
	bool eof( void ) const;
	// get active flags
	zconf::uint32 flags( void ) const;
	// number of bytes treated in the last operation
	zconf::uint64 gcount( void ) const;
	// number of bytes treated since the opening
	zconf::uint64 tcount( void ) const;
	// data buffer offset
	zconf::uint64 zoffset( void ) const;
	// get error string
	const std::string &error( void ) const;
	// read n bytes and allocate them on data
	zipentry &read( zconf::cbytep data, zconf::uint64 nbytes );
	// write n bytes on data
	zipentry &write( zconf::cbytep data, zconf::uint64 nbytes );

private:
	// private constructors
	zipentry( ziparchive::core &acore, file_info_32 &entry, zconf::uint32 flags );
	zipentry();

private:
	// class core structure declaration
	typedef struct core;
	// internal core structure
	core *_core;

public:
	// friend classes
	friend class ziparchive;

};

// zip timestamp
typedef struct zip_tm{
    zconf::uint16 tm_sec;               // seconds after the minute - [0,59]
    zconf::uint16 tm_min;               // minutes after the hour - [0,59]
    zconf::uint16 tm_hour;              // hours since midnight - [0,23]
    zconf::uint16 tm_mday;              // day of the month - [1,31]
    zconf::uint16 tm_mon;               // months since January - [0,11]
    zconf::uint16 tm_year;              // years - [1980..2044]
};

#endif /* ZIPARCHIVE_H_ */
