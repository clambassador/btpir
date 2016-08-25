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

#ifndef __BTPIR__DELIMINATED_PIR_DATABASE__H__
#define __BTPIR__DELIMINATED_PIR_DATABASE__H__

#include "build_database/pir_database_manifest_base.h"

#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>

#include "ib/logger.h"

using namespace std;
using namespace ib;

namespace btpir {

/* The DeliminatedPIRDatabase stores the 1st half of the data:
   the mapping from address to a list of blocks in the 2nd half of the database.
   The user first retrieves the list of blocks to get for their address,
   and then gets the actual blocks themselves.

   In this format of the database, the list of blocks is represented as a
   sequence of block ids. This means that the size of each entry is unknown:
   some may take up more space in the database. Consequently, entries may span
   over blocks and even span entire blocks.

   At the start of each block, a 4-byte number indicates how many bytes
   remain for the current address (whatever it is). If this is less than a
   blocksize, one jumps that distance to read the next address and can use this
   information to locate the address of interest.

   If there is not room for this address to be listed in this block (i.e., the
   space remaining after writting the current transaction bytes isn't enough for
   the full address, then (and only) is the 35-byte current address listed after the
   4-bytes bytes remaining number.
*/
class DeliminatedPIRDatabase : public PIRDatabaseManifestBase {
public:
	DeliminatedPIRDatabase(const string& directory, const string& filename)
		: PIRDatabaseManifestBase(directory, filename), _len_len(4) {
		_fmt = "format_2";
	}
	virtual ~DeliminatedPIRDatabase() {
	}

	virtual void build(const vector<string>& addresses,
			   const vector<string>& data) {
		size_t len = 0;
		for (auto &x : data) {
			len += x.length();
		}
		uint64_t db_size_bit = 8 * len;
		uint64_t pir_blocksize_bit = 16 + (uint64_t) sqrt(
			(long double) db_size_bit + 256);

		set_blocksize(pir_blocksize_bit / 8);
		trace();
		open_for_write();
		process_entries(addresses, data);
	}

protected:
	virtual void write_opening_header() {
		write_zeros(header_len());
		_cur_distance = header_len();
	}

	virtual size_t header_len() const {
		return _len_len;
	}

	virtual void write_header(uint32_t remaining) {
		_fout->write(reinterpret_cast<const char*>(&remaining),
		             sizeof(remaining));
		_total_size += header_len();
		_cur_distance = header_len();
		if (remaining > _pir_blocksize_bytes - header_len()
				 - _addr_len - footer_len()) {
			/* There will be no address on this block.
			 * so force the current one. */
			 _fout->write(_cur_addr.c_str(), _cur_addr.length());
			 _cur_distance += _cur_addr.length();
			 _total_size += _cur_addr.length();
		}
	}

	int _len_len;
};

}  // namespace bitcoin_pir

#endif  // __DELIMINATED_PIR_DATABASE__H__
