/*
	This file is part of EZXPack.

	Copyright (C) 2009 Gennadiy Brich <gennadiy.brich@gmail.com>

	EZXPack is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	EZXPack is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EZXPack.  If not, see <http://www.gnu.org/licenses/>.
*/



#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <list>
#include <algorithm>
#include <inttypes.h>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "ezxpack_cfg.h"
#include "ezxpack.hh"



using namespace std;
using namespace boost;
namespace po = program_options;



static const string g_MOTOSKIN_HEADER("motoskin");
static const string g_MOTOSKIN_VERSION("v1.00");
static const string g_COMMENTS_FILENAME("comments.txt");
static const string g_OUTPUT_DIRECTORY("resources");
static const string g_OUTPUT_FILENAME("iconres.ezx");
static const string g_DIRECTORY_DELIMITER("/");
static const uint32_t g_START_HEADER_MAGIC(0xabcddcba);
static const uint32_t g_DATA_ENTRY_MAGIC(0xdcbaabcd);



int main(int argc, char *argv[])
{
	int status = 0;

try
{
	// configure command-line options
	vector<string> input_filenames;
	po::options_description args_generic("Generic options");
	args_generic.add_options()
		("help,h", "produce help message")
		("verbose,v", "verbose output")
		("pack,p", "pack directory")
		("output,o", po::value<string>(), "output path")
	;
	po::options_description args_hidden("Hidden options");
	args_hidden.add_options()
		("input-file,i", po::value< vector<string> >(&input_filenames))
	;
	po::options_description args_all("All options");
	args_all.add(args_generic).add(args_hidden);
	po::positional_options_description pod;
	pod.add("input-file", -1);
	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).
			options(args_all).positional(pod).run(), vm);
	po::notify(vm);

	// output usage info
	if(vm.count("help") || input_filenames.size() != 1)
	{
		cout << "EZXPack v" << EZXPACK_VERSION_MAJOR << '.' << EZXPACK_VERSION_MINOR
			<< ", Copyright (C) 2009 Gennadiy Brich (G)" << endl << endl;

		cout << args_generic << endl;

		// examples
		cout << "Examples:" << endl;
		cout << "  " << "`" << argv[0] << " -o resources iconres.ezx` - extract \"iconres.ezx\" to \"resources\" directory" << endl;
		cout << "  " << "`" << argv[0] << " -p -o iconres.ezx resources` - pack \"resources\" directory to \"iconres.ezx\"" << endl;

		throw 1;
	}

	// perform actions
	cout << hex << setfill('0');
	if(vm.count("pack"))
	{
		motoskin_pack(input_filenames[0],
			vm.count("output") ? vm["output"].as<string>() : g_OUTPUT_FILENAME, (bool)vm.count("verbose"));
	}
	else
		motoskin_extract(input_filenames[0],
			vm.count("output") ? vm["output"].as<string>() : g_OUTPUT_DIRECTORY, (bool)vm.count("verbose"));
}
catch(int &e)
{
	status = e;
}

	return status;
}



void motoskin_extract(const string &arg_input_filename, const string &arg_output_dir, bool arg_verbose)
{
	ifstream iF;

try
{
	iF.open(arg_input_filename.c_str(), ios::binary);

	if(!iF.is_open())
		throw Exception(11, "can't open file " + arg_input_filename);

	// read file header & version
	{
		string header, version;
		iF >> header;
		if(arg_verbose)
			cout << "header: " << header << endl;
		iF >> version;
		if(arg_verbose)
			cout << "version: " << version << endl;

		if(header != g_MOTOSKIN_HEADER || version != g_MOTOSKIN_VERSION)
			throw Exception(12, "unsupported file format");
	}

	// read start header
	uint32_t start_offset;
	iF >> hex >> start_offset;
	if(arg_verbose)
		cout << "start header offset: 0x" << setw(8) << start_offset << endl;

	// skip one new line character
	iF.ignore(1);

	streampos comment_offset = iF.tellg();

	// read header
	iF.seekg(start_offset);
	start_header_t start_header;
	iF.read((char *)&start_header, sizeof(start_header));

	if(start_header.magic != g_START_HEADER_MAGIC)
		throw Exception(13, "incorrect start header magic");

	if(arg_verbose)
		cout << start_header << endl;

	// save comments
	{
		streampos start_pos = iF.tellg();
		iF.seekg(comment_offset);
		uint32_t buffer_size = start_header.id_table_offset - comment_offset;
		char *buffer = new char[buffer_size];
		iF.read(buffer, buffer_size);
		ofstream oF;
		oF.open(g_COMMENTS_FILENAME.c_str(), ios::binary);
		if(oF.is_open())
		{
			oF.write(buffer, buffer_size);
			oF.close();
		}
		delete [] buffer;
		iF.seekg(start_pos);
	}

	filesystem::create_directory(arg_output_dir);

	// iterate through hash table
	for(uint32_t i = 0; i < start_header.ht_entries_count; ++i)
	{
		ht_entry_t ht_entry;
		iF.read((char *)&ht_entry, sizeof(ht_entry));

		if(ht_entry.hashed_id == 0xffffffff)
			continue;

		// save get pointer
		streampos pos = iF.tellg();

		// read filename
		iF.seekg(ht_entry.id_offset + start_header.id_table_offset);
		string filename;
		iF >> filename;

		// extract file
		cout << "extracting " << filename << "... ";
		iF.seekg(ht_entry.data_offset + start_header.data_offset);
		data_entry_t data_entry;
		iF.read((char *)&data_entry, sizeof(data_entry));

		bool success = false;
		if(data_entry.magic == g_DATA_ENTRY_MAGIC)
		{
			char *buffer = new char[data_entry.file_length];
			iF.read(buffer, data_entry.file_length);

			ofstream oF;
			oF.open((arg_output_dir + g_DIRECTORY_DELIMITER + filename).c_str(), ios::binary);
			if(oF.is_open())
			{
				oF.write(buffer, data_entry.file_length);
				oF.close();
				success = true;
			}

			delete [] buffer;
		}

		cout << (success ? "done" : "failed") << endl;

		// restore get pointer
		iF.seekg(pos);
	}

	iF.close();
}
catch(Exception &e)
{
	cout << "error: " << e.what() << endl;

	switch(e.status())
	{
	default:
	case 12: iF.close();
	case 11: ;
	}

	throw e.status();
}
}


void motoskin_pack(const string &arg_input_dir, const string &arg_output_filename, bool arg_verbose)
{
	ofstream oF;

try
{
	if(!filesystem::is_directory(arg_input_dir))
		throw Exception(21, "can't open input directory");

	oF.open(arg_output_filename.c_str(), ios::binary);
	if(!oF.is_open())
		throw Exception(22, "can't open output file");

	// write header
	oF << g_MOTOSKIN_HEADER;
	write_unix_endl(oF);
	write_unix_endl(oF);
	oF << g_MOTOSKIN_VERSION;
	write_unix_endl(oF);

	// skip start offset
	streampos start_pos = oF.tellp();
	oF.seekp(8, ios::cur);
	write_unix_endl(oF);

	// write user comments
	if(filesystem::exists(g_COMMENTS_FILENAME) && filesystem::is_regular(g_COMMENTS_FILENAME))
	{
		ifstream iF;
		iF.open(g_COMMENTS_FILENAME.c_str(), ios::binary);
		if(iF.is_open())
		{
			uint32_t buffer_size = filesystem::file_size(g_COMMENTS_FILENAME);
			char *buffer = new char[buffer_size];
			iF.read(buffer, buffer_size);
			iF.close();
			oF.write(buffer, buffer_size);
			delete [] buffer;
		}
	}

	// fill list of files to pack
	list<string> files_to_pack;
	{
		filesystem::directory_iterator it_end;
		for(filesystem::directory_iterator it(arg_input_dir); it != it_end; ++it)
			if(!filesystem::is_directory(it->path().leaf()))
				files_to_pack.push_back(it->path().leaf());
	}
	files_to_pack.sort();

	start_header_t start_header;
	start_header.magic = g_START_HEADER_MAGIC;
	start_header.ht_entries_count = files_to_pack.size();
	start_header.id_table_offset = oF.tellp();

	// output filenames and fill some hash records parameters
	vector<ht_entry_t> hash_records(start_header.ht_entries_count);
	{
		uint32_t i = 0;
		for(list<string>::iterator it = files_to_pack.begin(); it != files_to_pack.end(); ++it)
		{
			hash_records[i].hashed_id = generate_hash(const_cast<char *>(it->c_str()));
			hash_records[i].id_offset = (uint32_t)oF.tellp() - start_header.id_table_offset;
			oF << *it;
			write_unix_endl(oF);
			++i;
		}
	}

	// be sure, that start header is 4-byte aligned
	oF.seekp(round_up((uint32_t)oF.tellp(), sizeof(uint32_t)));

	// fill start header text offset
	uint32_t start_offset = oF.tellp();
	oF.seekp(start_pos);
	oF << setfill('0') << hex << setw(8) << start_offset;
	oF.seekp(start_offset);
	if(arg_verbose)
		cout << "start header offset: 0x" << setw(8) << start_offset << endl;

	// write start header
	start_header.ht_offset = (uint32_t)start_offset + sizeof(start_header);
	start_header.data_offset = start_header.ht_offset + hash_records.size() * sizeof(ht_entry_t);
	if(arg_verbose)
		cout << start_header << endl;

	oF.write((char *)&start_header, sizeof(start_header));

	// write data
	oF.seekp(start_header.data_offset);
	{
		uint32_t i = 0;
		for(list<string>::iterator it = files_to_pack.begin(); it != files_to_pack.end(); ++it)
		{
			hash_records[i].data_offset = (uint32_t)oF.tellp() - start_header.data_offset;
			data_entry_t data_entry;
			data_entry.magic = g_DATA_ENTRY_MAGIC;

			data_entry.file_length = filesystem::file_size(filesystem::path(arg_input_dir) / *it);
			oF.write((char *)&data_entry, sizeof(data_entry));

			// write file
			{
				cout << "writing " << *it << "... ";

				bool success = false;
				ifstream iF;
				iF.open((arg_input_dir + g_DIRECTORY_DELIMITER + *it).c_str(), ios::binary);
				if(iF.is_open())
				{
					char *buffer = new char[data_entry.file_length];
					iF.read(buffer, data_entry.file_length);
					iF.close();
					oF.write(buffer, data_entry.file_length);
					delete [] buffer;
					success = true;
				}

				cout << (success ? "done" : "failed") << endl;
			}

			++i;
		}
	}
	oF.seekp(start_header.ht_offset);

	// write hash table
	{
		uint32_t hash_records_count = hash_records.size();
		for(uint32_t i = 0; i < hash_records_count; ++i)
			oF.write((char *)&hash_records[i], sizeof(hash_records[i]));
	}

	oF.close();
}
catch(Exception &e)
{
	cout << "error: " << e.what() << endl;

	switch(e.status())
	{
	default:
	case 23: oF.close();
	case 22: ;
	case 21: ;
	}

	throw e.status();
}
}


int generate_hash(char *c)
{
    int hv = 0;
    while(*c)
    {
        hv = (hv << 1) ^ (hv >> 20) ^ *(c++);
    }

    return hv;
}


template <typename T>
T round_up(T arg_value, unsigned int arg_base)
{
	--arg_base;
	return (arg_value + arg_base) & ~arg_base;
}


void write_unix_endl(ofstream &ret_of)
{
	const char *eol = "\n";
	ret_of.write(eol, 1);
}


ostream &operator<<(ostream &ret_os, const start_header_t &arg_header)
{
	ret_os << "magic: 0x" << setw(8) << arg_header.magic << endl;
	ret_os << "id table offset: 0x" << setw(8) << arg_header.id_table_offset << endl;
	ret_os << "hashtable entries count: " << dec << arg_header.ht_entries_count << hex << endl;
	ret_os << "hashtable offset: 0x" << setw(8) << arg_header.ht_offset << endl;
	ret_os << "data offset: 0x" << setw(8) << arg_header.data_offset;

	return ret_os;
}
