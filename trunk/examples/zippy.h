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

#ifndef ZIPPY_H_
#define ZIPPY_H_

#include <ziparchive.h>

#include <fstream>

class zippy{

private:
    // instance
    static zippy *_instance;

public:
    // get instance
    static zippy &get();
	// main entry
	int main( int argc, char *argv[] );
	// extract entry to stdout
	void extract_entry( const std::string &entrystr );

public:
	// destructor
	virtual ~zippy();

private:
	zippy( void );
    // print options
    void print_usage( void );

private:
    // archivo zip
	ziparchive zip;
	// create buffer
	zconf::bytep _data;

};

#endif //ZIPPY_H_
