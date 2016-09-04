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
#include "build_database/pir_database_base.h"

using namespace std;
using namespace ib;

namespace btpir {

class TransactionPIRDatabase : public PIRDatabaseBase {
public:
	TransactionPIRDatabase(uint64_t blocksize,
			       const string& directory,
			       const string& filename)
		: PIRDatabaseBase(directory, filename),
		  _len_len(4) {
		_pir_blocksize_bytes = blocksize;
		_blocksize_useable = _pir_blocksize_bytes
			- header_len() - footer_len();
	}

	virtual ~TransactionPIRDatabase() {
	}

	virtual void build(const vector<string>& addresses,
			   const vector<string>& data) {
		assert(0);
	}

	virtual void build(const vector<string>& entries,
			   map<uint64_t, set<uint32_t>> *pos_to_blocks) {
		string tmp_file = Logger::stringify("%_%.pir",
 	                                            _filename,
					            _pir_blocksize_bytes);
		_fout.reset(new ofstream(tmp_file));

		write_zeros(header_len());
		_cur_distance = header_len();
		_total_size = header_len();
		_cur_block = 0;

		process_entries(entries, pos_to_blocks);
	}

protected:

	/* process entries for this database needs only the data chunks
	 * themselves (entries). It also takes a map from the position in the
	 * entries to the set of blocks corresponding to the range in the PIR
	 * database where that entry is stored. This is used to build a database
	 * to look this up.
	 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
	virtual void process_entries(
			const vector<string>& entries,
			map<uint64_t, set<uint32_t>> *pos_to_blocks) {
		size_t pos = 0;
		for (const auto &x : entries) {
			start_tx(x, x.length());
			uint32_t length = x.length();
			write(reinterpret_cast<const char*>(
				&length), sizeof(uint32_t));
			write(x);
			(*pos_to_blocks)[pos] = _blocks_used;
			end_tx(x, x.length());
			++pos;
		}
		assert(_fout->good());
	}
#pragma clang diagnostic pop

	virtual void start_tx(const string& address, uint32_t length) {
		if (get_safe_len() < header_len()) {
			write_zeros(get_safe_len());
			new_block(0);
		}

		_blocks_used.clear();
	}

	virtual size_t header_len() const {
		return _len_len;
	}

	virtual void write_footer(uint32_t remaining) {
		_fout->write(reinterpret_cast<const char*>(&remaining),
		             sizeof(remaining));
	}

	int _len_len;
};

}  // namespace bitcoin_pir

#endif  // __TRANSACTION_PIR_DATABASE__H__
