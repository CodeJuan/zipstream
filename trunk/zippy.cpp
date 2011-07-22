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

#include "zippy.h"

#include <sstream>

zippy *zippy::_instance = 0;

zippy &zippy::get(){
    if( _instance == 0 ){
        _instance = new zippy();
    }
    return *_instance;
}

int zippy::main( int argc, char*argv[] ){
	/*
	 * quick example of how to use the library:
	 * please: don't program this way!!
	 */

	std::string entry    = "";
	std::string zip_file = "";
	bool extract_all     = false;
	bool print_entries   = false;

	// get arguments
	if( argc == 1 ){
		print_usage(); return 0;
	}

	for( int narg=2;narg<=argc;narg++ ){
		if( std::string( argv[narg-1] ).length() > 1 && !std::string( argv[narg-1] ).substr(0,1).compare( "-" ) ){
			for( int nsarg=1;nsarg<std::string( argv[narg-1] ).length();nsarg++ ){
				if( !std::string( argv[narg-1] ).substr(nsarg,nsarg).compare( "a" ) ){
					if( zip.is_open() || print_entries ){
						print_usage(); return 1;
					}
					extract_all = true;
				}else if( !std::string( argv[narg-1] ).substr(nsarg,nsarg).compare( "t" ) ){
					if( zip.is_open() || extract_all ){
						print_usage(); return 1;
					}
					print_entries = true;
				}else{
					std::cerr << "invalid argument: ";
					std::cerr << std::string( argv[narg-1] ).substr(nsarg,nsarg) << std::endl;
					return 1;
				}
			}
		}else if( narg+1 == argc ){
			if( print_entries || extract_all ){
				print_usage(); return 1;
			}
			zip_file = argv[narg-1];
		}else if( narg == argc ){
			if( print_entries || extract_all ){
				zip_file = argv[narg-1];
			}else{
				entry = argv[narg-1];
			}
		}else{
			std::cerr << "invalid argument: ";
			std::cerr << argv[narg-1] << std::endl;
			return 1;
		}
	}

	// check if the zip was found
	if( !zip.open( zip_file.c_str() ).is_open() ){
		std::cerr << "error: the zip file wasn't found, it's corrupted or it's not compatible" << std::endl;
		return 1;
	}else{
		// extract entries
		if( print_entries ){
			if( zip.is_open() ){
				std::vector<std::string> entries = zip.entries();
				// read entries
				for( int i=0;i<entries.size();i++ ){
					std::cout << entries[i] << std::endl;
				}
			}else{
				std::cout << zip.error() << std::endl;
			}
		}else if( extract_all ){
			if( zip.is_open() ){
				std::vector<std::string> entries = zip.entries();
				// read entries
				for( int i=0;i<entries.size();i++ ){
					std::cout << "[zippy::" << entries[i] << "]" << std::endl;
					extract_entry( entries[i] );
				}
			}else{
				std::cout << zip.error() << std::endl;
			}
		}else if( !entry.empty() ){
			extract_entry( entry );
		}
	}

	return 0;
}

zippy::zippy( void ){
	_data = new zconf::byte[ ( 1 << 10 )  + 1 ];
}

zippy::~zippy(){
	delete _data;
}

void zippy::print_usage( void ){
	std::cout << "utility to import and export zip32 entries from/to cin/cout"       << std::endl;
	std::cout << "author: Victor Egea Hernando, email: egea.hernando@gmail.com"      << std::endl;
	std::cout << std::endl << "zippy [options] zipfile [entry]"                      << std::endl;
	std::cout << std::endl << "options:"                                             << std::endl;
	std::cout              << "   -a extract all the zip entries"                    << std::endl;
	std::cout              << "   -d compact zip entries / defrag"                   << std::endl;
	std::cout              << "   -t list zip entries"                               << std::endl;
	std::cout              << "   -r remove zip entry"                               << std::endl;
	std::cout << std::endl << "examples:"                                            << std::endl;
	std::cout              << "   1) Extract all zip entries to cout"                << std::endl;
	std::cout              << "   $ zippy -a base.zip "                              << std::endl;
	std::cout << std::endl << "   2) List all zip entries"                           << std::endl;
	std::cout              << "   $ zippy -t base.zip "                              << std::endl;
	std::cout << std::endl << "   3) Extract selected zip entry"                     << std::endl;
	std::cout              << "   $ zippy base.zip moby-dick.txt > moby-dick.txt"    << std::endl;
}

void zippy::extract_entry( const std::string &entrystr ){
	// open entry
	zipentry *entry = zip.entry( entrystr.c_str() );
	// check if it was found
	if( entry ){
		// read and output entry
		while( !entry->eof() && !( entry->flags() & zstream::ferr ) ){
			entry->read( _data, 1 << 10 );
			_data[ entry->gcount() ] = '\0';
			std::cout << _data;
		}
		// output possible errors
		if( entry->flags() & zstream::ferr ){
			std::cerr << std::endl << std::endl << entry->error() << std::endl;
		}
		// close entry
		entry->close();
	}else{
		std::cerr << "error: entry '" << entrystr << "' not found" << std::endl;
	}
}

int main( int argc, char *argv[] ){
	return zippy::get().main( argc, argv );
}
