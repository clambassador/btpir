/*
   Copyright 2016, Joel Reardon, UC Berkeley

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef __TRANSACTION_PIR_DATABASE__H__
#define __TRANSACTION_PIR_DATABASE__H__

#include <fstream>
#include <string>
#include <vector>

#include "ib/logger.h"

using namespace std;
using namespace ib;

namespace btpir {

/* TODO: integrate into the base class */

class TransactionPIRDatabase {
public:
	TransactionPIRDatabase(uint64_t blocksize)
		: _blocksize(blocksize), _blocks(0),
		  _len_len(4) {
		_blocksize_useable = _blocksize - header_len()
					  	- footer_len();
	}

	~TransactionPIRDatabase() {
		while (get_safe_len()) {
			do_write("\0", 1);
		}

		/* Increment the address by one for "infinity" */

		_total_size += footer_len();
		_cur_distance += footer_len();
		_fout->close();
		string full_file = Logger::stringify("%_%_%.pir",
 	                                      	     _filename,
					      	     _blocks,
                                              	     _blocksize);
		Logger::info("renaming temp database % to %",
			     _tmp_file, full_file);
		assert(!rename(_tmp_file.c_str(), full_file.c_str()));
		Logger::info("Wrote PIR DB");
		Logger::info("Total size (B): %", _total_size);
		Logger::info("Blocks        : %", _blocks);
		Logger::info("Blocksize  (B): %", _blocksize);
	}

	void open_for_write(const string& filename) {
		_filename = filename;
		_tmp_file = Logger::stringify("%_%.pir",
 	                                      filename,
                                              _blocksize);
		_fout.reset(new ofstream(_tmp_file));
		string zeros;
		for (size_t i = 0; i < header_len(); ++i) {
			zeros += '\0';
		}
		*_fout << zeros;


		_cur_distance = header_len();
		_total_size = header_len();
		_cur_block = 0;

		assert(_fout->good());
	}

	void start_tx(uint32_t length) {
		if (get_safe_len() < header_len()) {
			while (get_safe_len()) {
				char c = 0;
				write(&c, 1);
			}
			new_block(0);
		}
		_blocks_used.clear();
	}

	size_t total_written() {
		return _total_size;
	}

	const set<uint32_t>& end_tx() {
		return _blocks_used;
	}

	void write(const char* data, size_t len) {
		size_t written = 0;
		while (len) {
			size_t pivot = get_safe_len();
			if (len <= pivot) {
				do_write(data + written, len);
				_blocks_used.insert(_cur_block);
				return;
			}
			if (pivot) {
				do_write(data + written, pivot);
				_blocks_used.insert(_cur_block);
				written += pivot;
				len -= pivot;
			}
			new_block(len);
		}
	}
protected:
	size_t remaining() {
		return _blocksize - _cur_distance;
	}

	size_t footer_len() {
		return 0;
	}

	size_t header_len() {
		return _len_len;
	}

	size_t compute_blocks() {
		size_t count = 0;
		size_t len = _len;
		len -= remaining();
		while (len > _blocksize_useable) {
			len -= _blocksize_useable;
			++count;
		}
		if (len) ++count;
		return count;
	}

	void new_block(uint32_t remaining) {
		_fout->write(reinterpret_cast<const char*>(&remaining),
		             sizeof(remaining));
		++_cur_block;
		_cur_distance = header_len();
		_total_size += footer_len() + header_len();
		++_blocks;
	}

	size_t get_safe_len() {
		return remaining() - footer_len();
	}

	void do_write(const char* data, size_t len) {
		if (!len) return;
		_fout->write(data, len);
		assert(_fout->good());
		_cur_distance += len;
		_total_size += len;
		assert(_cur_distance <= _blocksize);
	}

	unique_ptr<ofstream> _fout;
	string _tmp_file;
	string _filename;
	uint64_t _cur_distance;
	uint64_t _total_size;
	uint64_t _blocksize;
	uint64_t _blocksize_useable;
	uint64_t _blocks;
	uint64_t _len;
	int _len_len;
	set<uint32_t> _blocks_used;
	int _cur_block;
};

}  // namespace bitcoin_pir

#endif  // __PIR_DATABASE__H__
