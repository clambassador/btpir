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

#ifndef __BTPIR__BUILD_DATABASE__AUTO_DELIMINATED_PIR_DATABASE__H__
#define __BTPIR__BUILD_DATABASE__AUTO_DELIMINATED_PIR_DATABASE__H__

#include "build_database/pir_database_manifest_base.h"

#include <fstream>
#include <string>
#include <vector>

#include "ib/logger.h"

using namespace std;
using namespace ib;

namespace btpir {

/* The AutoDeliminatedPIRDatabase stores the 1st half of the data:
   the mapping from address to a list of blocks in the 2nd half of the database.
   The user first retrieves the list of blocks to get for their address, and
   then gets the actual blocks themselves.

   In this format of the database, the list of blocks is represented with a
   binary string corresponding to a yes/no decision on that particular block.
   As such each block has the same size and is therefore auto deliminated.
 */
class AutoDeliminatedPIRDatabase : public PIRDatabaseManifestBase {
public:
	/* Creates an auto deliminated (format 1) PIR database.
	 * @directory: the directory where the output is stored
	 * @filename: the PIR database name
	 * @address_list: a vector of bitcoin addresses
	 * @data: a vector of database entries
	 * Note that the two vectors @address_list and @data have the same size
	 * and correspond to each other.
	 */
	AutoDeliminatedPIRDatabase(const string& directory,
				   const string& filename)
			: PIRDatabaseManifestBase(directory, filename) {
	}

	/* Destructor finishes writing the database. It fills the final block's
	 * leftover content with zeros and closes the file.
	 */
	virtual ~AutoDeliminatedPIRDatabase() {
	}

	virtual void build(const vector<string>& addresses,
			   const vector<string>& data) {
		assert(data.size() == addresses.size());
		_len = data[0].length();
		for (auto &x : data) {
			assert(x.length() == _len);
		}

		set_blocksize(_len);
		trace();
		open_for_write();
		process_entries(addresses, data);
	}

protected:
	virtual void pre_write() {}
	virtual void post_write() {}

        virtual void start_tx(const string& address, uint32_t length) {
	}
	/* In the auto-deliminated, the new block is called before starting
	   the next write, and we want the address about the previous one.
	 */
	virtual void end_tx(const string& address, uint32_t length) {
		_cur_addr = address;
	}



	virtual size_t footer_len() const {
		return 0;
	}

	virtual size_t header_len() const {
		return 0;
	}

	virtual void write_opening_header() {
	}

	virtual void write_closing_footer() {
		new_block(0);
	//	Logger::alert("lirst %", _cur_addr);
	//	*_fmanifest << _cur_addr << endl;
	}

	/* Writes @len bytes of the the string @data to the current output file
	 * _fout. All writes shall go through this function.
	 */
	void do_write(const char* data, size_t len) {
		if (!len) return;
		_fout->write(data, len);
		assert(_fout->good());
		_cur_distance += len;
		_total_size += len;
		assert(_cur_distance <= _pir_blocksize_bytes);
	}
};

}  // namespace btpir

#endif  // __BTPIR__BUILD_DATABASE__DELIMINATED_PIR_DATABASE__H__
