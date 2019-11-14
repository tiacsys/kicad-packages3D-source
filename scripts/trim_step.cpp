/*
 * trim_step2.cpp
 *
 *  Created on: Jan 19, 2018
 *      Author: seth
 */

#include <algorithm>
#include <unordered_map>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <regex>
#include <iostream>

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}
// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}
template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss(s);
    std::string item;
    if (std::getline(ss, item, delim)) {
		*(result++) = item;
	}
    if (std::getline(ss, item)) {
		*(result++) = item;
	}
}

int main( int argc, char **argv)
{
	std::string input_file(argv[1]);
	std::string output_file(argv[2]);

	std::vector<std::string> in_lines;
	std::vector<std::string> out_lines;

	std::ifstream is(input_file);
	
  	auto n_lines = std::count(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>(), '\n') + 1;
	is.seekg( 0 );
	in_lines.reserve( n_lines );
	out_lines.reserve( n_lines );

	std::vector<std::string> footer;
	std::vector<std::string> header;
	bool past_header = false;
	bool past_data = false;
	bool continuing = false;

	while( is )
	{
		std::string line;
		std::getline( is, line );

		if( past_header )
		{
			if( past_data || line.find( "ENDSEC;") != std::string::npos )
			{
				past_data = true;
				footer.emplace_back( line );
			}
			else
			{
				rtrim(line);

				if( continuing )
				{
					if( std::isalpha(line.front()) )
						out_lines.back().append( " " );
					
					out_lines.back() += line;
				}
				else
				{
					out_lines.emplace_back( line );
				}

				continuing = ( line.back() != ';' );
			}			
		}
		else
		{
			if( line.find( "DATA;") != std::string::npos )
				past_header = true;

			header.emplace_back( line );
		}
		
	}

	is.close();


	std::unordered_map<std::string, unsigned> uniques;
	std::unordered_map<unsigned, unsigned> lookup;
	std::regex e ("#([0-9]+)");

	do
	{

		in_lines = out_lines;
		out_lines.clear();
		uniques.clear();
		lookup.clear();

		for( const auto& line : in_lines )
		{
		
			std::vector<std::string> elems;
			split(line, '=', std::back_inserter(elems));

			// remove the #
			elems[0].erase(0,1);
			unsigned oldnum = std::stol(elems[0]);

			ltrim(elems[1]);
			rtrim(elems[1]);

			auto it = uniques.emplace( std::make_pair(elems[1], out_lines.size() + 1 ) );

			while( ( elems[1].find("PRODUCT_DEFINITION") == 0
					|| elems[1].find("SHAPE_REPRESENTATION") == 0 ) && !it.second )
			{
				elems[1].append( " " );
				it = uniques.emplace( std::make_pair(elems[1], out_lines.size() + 1 ) );
			}

			if( !it.second )
			{
				if( elems[1] != it.first->first )
					std::cout << "Warning! " << elems[1] << " is not the same as " << it.first->first << std::endl;
				// didn't insert!
				lookup.emplace( oldnum, it.first->second );
			}
			else
			{
				lookup.emplace( std::make_pair( oldnum, out_lines.size() + 1 ) );
				out_lines.emplace_back( "#" + std::to_string( out_lines.size() + 1 ) + "=" + elems[1]);
			}
		}

		for( size_t i = 0; i < out_lines.size(); i++ )
		{
			std::vector<std::string> elems;
			std::smatch m;
			auto& line = out_lines[i];

			split(line, '=', std::back_inserter(elems));

			line = elems[0] + "=";
			
			while( std::regex_search( elems[1], m, e, std::regex_constants::match_not_null) )
			{
				if( m.empty() ) 
					break;

				unsigned oldval = std::stol( m[1] );
				auto newval = lookup.find( oldval );

				if( newval == lookup.end() )
					line += ( m.prefix().str() + "#" + m[1].str() );	
				else
					line += ( m.prefix().str() + "#" + std::to_string( newval->second ) );

				elems[1] = m.suffix();
			}

			if( !elems[1].empty() )
				line += elems[1];
		}

	} while( in_lines.size() > out_lines.size() );

	std::ofstream os(output_file);

	for( const auto& line : header )
		os << line << std::endl;

	for( const auto& line : out_lines )
		os << line << std::endl;

	for( const auto& line : footer )
		os << line << std::endl;

	os.close();

	std::cout << "Finished: " << input_file << " " << n_lines << " shrunk to " << out_lines.size() + header.size() + footer.size() << std::endl;
} 
