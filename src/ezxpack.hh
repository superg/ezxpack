/*
	This file is part of ezxpack.

	Copyright 2009 Gennadiy Brich

	ezxpack is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	ezxpack is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with ezxpack.  If not, see <http://www.gnu.org/licenses/>.
*/



#ifndef EZXPACK_HH__
#define EZXPACK_HH__



#include <string>
#include <exception>



using namespace std;



struct start_header_t
{
	uint32_t magic;
	uint32_t id_table_offset;
	uint32_t ht_entries_count;
	uint32_t ht_offset;
	uint32_t data_offset;
};

struct ht_entry_t
{
	uint32_t hashed_id;
	uint32_t id_offset;
	uint32_t data_offset;
};

struct data_entry_t
{
	uint32_t magic;
	uint32_t file_length;
};


class Exception : public exception
{
public:
	int m_Status;
	string m_Message;

	Exception(int arg_status, string arg_message) throw() :
		m_Status(arg_status), m_Message(arg_message) {}
	virtual ~Exception() throw() {}
	virtual int status() const throw()
	{
		return m_Status;
	}
	virtual const char* what() const throw()
	{
		return m_Message.c_str();
	}
};


void motoskin_extract(const string &arg_input_filename, const string &arg_output_dir, bool arg_verbose);
void motoskin_pack(const string &arg_input_dir, const string &arg_output_filename, bool arg_verbose);
int generate_hash(char *c);

template <typename T>
T round_up(T arg_value, unsigned int arg_base);

void write_unix_endl(ofstream &ret_of);

ostream &operator<<(ostream &ret_os, const start_header_t &arg_header);



#endif
