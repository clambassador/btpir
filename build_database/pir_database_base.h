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

#ifndef __BTPIR__BUILD_DATABASE__PIR_DATABASE_BASE__H__
#define __BTPIR__BUILD_DATABASE__PIR_DATABASE_BASE__H__

#include "build_database/abstract_pir_database.h"

#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "ib/logger.h"

using namespace std;
using namespace ib;

namespace btpir {

/* The PIRDatabaseBase is a base class for PIR databases. It covers the basic
 * routines to store data, retrieve sizes, and output the data to a file.
 */
class PIRDatabaseBase : public AbstractPIRDatabase {
public:
	PIRDatabaseBase(const string& directory,
		        const string& filename)
		: _len(0), _blocks(0),
		  _addr_len(35),
		  _filename(directory + "/" + filename),
		  _fmt("base") {
	}
	/* Destructor finishes writing the database. It fills the final block's
	 * leftover content with zeros and closes the file.
	 */
	virtual ~PIRDatabaseBase() {
		write_zeros(get_safe_len());
		write_closing_footer();
		_fout->close();

		Logger::info("(btpir) Wrote % PIR DB: %", _fmt, _filename);
		Logger::info("(btpir) Total size (B): %", _total_size);
		Logger::info("(btpir) PIR Blocks    : %", _blocks);
		Logger::info("(btpir) Blocksize  (B): %", _pir_blocksize_bytes);

		/* rename files to have useful data handy */
                string old_filename = Logger::stringify("%_%.pir",
                                                        _filename,
                                                        _pir_blocksize_bytes);

                string new_filename = Logger::stringify("%_%_%.pir",
                                                        _filename,
                                                        _blocks,
                                                        _pir_blocksize_bytes);
                char buf[1024];
                getcwd(buf, 1024);
                Logger::info("cwd is %", buf);
                assert(!rename(old_filename.c_str(), new_filename.c_str()));
                assert(!rename((old_filename + ".manifest").c_str(),
                               (new_filename + ".manifest").c_str()));
	}

protected:
	/* called when writing the first PIR block's header */
	virtual void write_opening_header() {
	}

	/* called when writing the last PIR block's footer */
	virtual void write_closing_footer() {
		_total_size += footer_len();
		_cur_distance += footer_len();
	}

	/* open the PIR database files for writing data. */
	virtual void open_for_write() {
		_fout.reset(new ofstream(
			Logger::stringify("%_%.pir",
					  _filename,
					  _pir_blocksize_bytes)));
		assert(_fout->good());

		/* TODO: sort this for nonbase ones */
		_fmanifest.reset(new ofstream(
			Logger::stringify("%_%.pir.manifest",
					  _filename,
					  _pir_blocksize_bytes)));
		assert(_fmanifest->good());
		_cur_distance = header_len();
		_total_size = header_len();
		_cur_block = 0;

		write_opening_header();
	}

	/* Called whenever a new transaction is being added to the database.
         * @address is address it is linked to, length is the length of
         * the data corresponding to the transaction.
         */
	virtual void start_tx(const string& address, uint32_t length) {
		_cur_addr = address;
	}

	/* Returns the total data that has been written to the PIR database.
         */
	virtual size_t total_written() {
		return _total_size;
	}

	/* Called whenever a transaction is finished being processed.
         */
	virtual void end_tx(const string& address, uint32_t length) {}

	/* Called before each atomic write to the database. */
	virtual void pre_write() {}

	/* Called after each atomic write to the database. */
        virtual void post_write() {}

	/* Returns the size of the footer. */
        virtual size_t footer_len() const {
                return 0;
        }

	/* Returns the size of the header. */
        virtual size_t header_len() const {
                return 0;
        }

	/* Writes the footer for the PIR block.
	   @remaining: bytes remaining on the transaction
	 */
        virtual void write_footer(uint32_t remaining) {}

	/* Writes the header for the PIR block.
	   @remaining: bytes remaining on the transaction
	 */
        virtual void write_header(uint32_t remaining) {}

	/* Sets the blocksize for the PIR database.
	   @blocksize: blocksize in bytes.
	 */
	virtual void set_blocksize(size_t blocksize) {
                _pir_blocksize_bytes = blocksize;
		_blocksize_useable = _pir_blocksize_bytes - header_len()
		                     - footer_len();
	}

	/* trace(): outputs geometry of the database. */
	virtual void trace() const {
                Logger::info("(btpir) DB size  (B) : %", _len);
                Logger::info("(btpir) Blk size (B) : %", _pir_blocksize_bytes);
	}

	/* process_entries(): takes a vector of address and data to store in the
	 * database. This corresponds to the data to be stored. The PIR database
	 * will be the interleaving of addresses and data, along with
	 * header/footer data at PIR block edges.
	 */
	virtual void process_entries(const vector<string>& addresses,
                           	     const vector<string>& data) {
		assert(data.size());
		assert(data.size() == addresses.size());

		Logger::info("processing % entries, first %",
			     data.size(), addresses[0]);
	        for (size_t i = 0; i < data.size(); ++i) {
			start_tx(addresses[i], data[i].length());
			write(data[i]);
			end_tx(addresses[i], data[i].length());
		}
	}

	/* remaining(): returns the bytes still remaining on the PIR block. */
	virtual size_t remaining() const {
		return _pir_blocksize_bytes - _cur_distance;
	}

	/* new_block(): called whenever a new PIR block is created. */
	virtual void new_block(size_t remaining) {
		write_footer(remaining);
		*_fmanifest << _cur_addr << endl;
		assert(_fmanifest->good());
		++_cur_block;
		_cur_distance = header_len();
		_total_size += footer_len() + header_len();
		++_blocks;
		write_header(remaining);
	}

	/* get_safe_len(): gets the writable bytes in the PIR block. */
	virtual size_t get_safe_len() const {
		return remaining() - footer_len();
	}

	/* write(): writes @data to the PIR database. */
	virtual void write(const string& data) {
		write(data.c_str(), data.length());
	}

	/* write(): writes @len bytes from @data to the PIR database. */
	virtual void write(const char* data, size_t len) {
		size_t written = 0;

		if (get_safe_len() == 0) new_block(0);
		assert(get_safe_len());
		while (len) {
			size_t pivot = get_safe_len();
			assert(pivot <= _pir_blocksize_bytes);
                        if (len <= pivot) {
                                safe_write(data + written, len);
                                return;
                        }
                        safe_write(data + written, pivot);
                        written += pivot;
                        len -= pivot;
                        new_block(len);
		}
	}

	/* write_zeros(): writes @len bytes of zeros to the database */
	virtual void write_zeros(size_t len) {
		string zeros;
		zeros.resize(len);
		_fout->write(zeros.c_str(), zeros.length());
	}

	/* Writes @len bytes of the the string @data to the current output file
	 * _fout. All writes shall go through this function.
	 */
	virtual void safe_write(const char* data, size_t len) {
		pre_write();
		if (!len) return;
		_fout->write(data, len);
		assert(_fout->good());
		_cur_distance += len;
		_total_size += len;
		assert(_cur_distance <= _pir_blocksize_bytes);
		post_write();
	}

	unique_ptr<ofstream> _fout;
	unique_ptr<ofstream> _fmanifest;
	string _cur_addr;
	uint64_t _pir_blocksize_bytes;
	uint64_t _cur_distance;
	uint64_t _total_size;
	uint64_t _blocksize_useable;
	uint64_t _len;
	uint64_t _blocks;
	uint64_t _addr_len;
	int _cur_block;
	string _filename;
	string _fmt;
};

}  // namespace bitcoin_pir

#endif  // __DELIMINATED_PIR_DATABASE__H__
