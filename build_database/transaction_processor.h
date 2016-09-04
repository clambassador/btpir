/*
   Copyright 2016
   Joel Reardon, UC Berkeley

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

#ifndef __BTPIR__BUILD_DATABASE__TRANSACTION_PROCESSOR__H__
#define __BTPIR__BUILD_DATABASE__TRANSACTION_PROCESSOR__H__

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "ib/logger.h"
#include "build_database/auto_deliminated_pir_database.h"
#include "build_database/deliminated_pir_database.h"
#include "build_database/transaction_pir_database.h"

using namespace ib;
using namespace std;

namespace btpir {

/* TransactionProcessor is a class that collects transactions, sent in any
 * order, and sorts them by addresses. It then outputs the PIR database that
 * correspond to this set of transactions.
 */
class TransactionProcessor {
public:
	TransactionProcessor(const string& directory, const string& filename)
		: _directory(directory), _filename(filename),
		  _db_size(0), _pos(0), _pir_blocks(0), _pir_blocksize(0),
		  _tx_data_sum(0), _shortaddr_len(20),
		  _longaddr_len(35), _len_len(4), _pirdb_pos(0) {}

	TransactionProcessor() : TransactionProcessor("", "") {}

	/* destructor of the transaction processor does the outputting of all
	 * the data that has been accumulated. If no filename is provided than
	 * nothing is written.
	 */
	virtual ~TransactionProcessor() {
		if (!_filename.empty()) {
			Logger::info("(txproc) Sum of TX data %", _tx_data_sum);
			output_db();

			// create an empty file that stores raw transaction size
			ofstream(Logger::stringify("%/%_raw_tx_size_%",
						   _directory, _filename,
						   _tx_data_sum));
			output_addr_len();
		} else {
			Logger::error("Filename is empty. Nothing written.");
		}
	}

	/* add_tx() stores a new transaction. Each transaction should call this
	 * function so that it is stored in the PIR database.
	 * @address: a set of (35-byte) addresses that are involved in this
	 *	     transaction.
	 * @transaction_data: the raw transaction data
	 */
	void add_tx(const set<string>& addresses,
		    const string& transaction_data) {
		static int max_addr_len = 0;
		_tx_data_sum += transaction_data.length();
		_txs.push_back(transaction_data);
		/* For each address that will read this transaction,
		   add the transaction length to its counter and the current
		   position in the sequence of data to build the blocks-to-get
		   database.
		 */
		for (auto &x : addresses) {
			add_address(x);
			_addr_to_tx_len[x] += transaction_data.length();
			_addr_to_positions[get_short_address(x)].insert(_pirdb_pos);

			/* if max_addr_len is unset, set it to the first
			 * address. Otherwise check that they are equal.
			 */
			if (max_addr_len == 0) max_addr_len = x.length();
			assert(max_addr_len == x.length());
			assert(x.length());
		}
		_pos += _len_len + transaction_data.length();
		++_pirdb_pos;
	}

	/* output_addr_len outputs data we can use in analysis of PIR
	 * performance. It outputs, for each address, the bytes of data
	 * for all its transactions and the number of PIR blocks that must be
	 * retrieved.
	 * @filename: the name of the file to store this data
	 */
	void output_addr_len() {
		string filename = Logger::stringify("%/%_address_to_tx_len",
			      _directory, _filename);
		ofstream fout(filename);
		assert(fout.good());
		for (auto &x : _addr_to_tx_len) {
			fout << x.first << " " << x.second
			     << " " << _addr_to_blocks[x.first].size()
			     << endl;
			assert(fout.good());
		}
	}

	virtual void set_main_pir_blocksize(uint64_t pir_blocksize) {
		_pir_blocksize = pir_blocksize;
	}

	/* output_db(): performs the work of outputting the PIR database.
	 * Assumes that no more transactions will be reported to the class. It
	 * creates both the main and the address database.
	 * @pir_blocksize: the blocksize used for the database. If this is 0,
	 * then it is computed using the approximate squareroot of the database
	 * size.
	 */
	void output_db() {
		string filename = Logger::stringify("%/%_default_blocksize",
						    _directory, _filename);

		uint64_t db_size_bit = 8 * _pos;
		_db_size = _pos;
		_pir_blocks = 0;

		if (_pir_blocksize == 0) {
			_pir_blocks = (uint64_t) sqrt((long double) db_size_bit);

			// Distances are 4-byte integers, though 2-byte
			// would probably be sufficient.
			_db_size += 4 * _pir_blocks;
			db_size_bit = 8 * _db_size;
			_pir_blocksize = _db_size / _pir_blocks;
		}
		else {
			_pir_blocks = _db_size / (_pir_blocksize - 4) + 1;
			_db_size += 4 * _pir_blocks;
		}

		Logger::info("Writing PIR DB : %_%", filename, _pir_blocksize);
		Logger::info("DB size     (B): %", _db_size);
		Logger::info("PIR blksize (B): %", _pir_blocksize);
		Logger::info("DB size   (MiB): %", _db_size >> 20);
		Logger::info("PIR block (MiB): %", _pir_blocksize >> 20);
		Logger::info("PIR blocks     : %", _pir_blocks);
		assert(_pir_blocksize > 4);

		{
		TransactionPIRDatabase short_db(_pir_blocksize,
						_directory,
						Logger::stringify("%_%.pir",
								  filename,
								  _pir_blocksize));
		short_db.build(_txs, &_pos_to_blocks);
		}

		make_skip_list();
		remap_addresses();
		trace();
		output_address_formats();
		output_address_manifest();
	}

protected:
	/* make_skip_list() creates a list of bad addresses for PIR. This means
	 * that they consume so many blocks that having them in the primary
	 * database unnessessary because the owner of these addresses will not
	 * benefit from having PIR---they will need to transmit more information
	 * than just keeping a block synced and they are unlikely to use small
	 * clients.
	 *
	 * Currently this was generated by inspection.
	 * TODO: a two pass approach that does this automatically for any
	 * address that uses more then sqrt. database size blocks.
	 */
	virtual void make_skip_list() {
		_skip_list.insert("001VayNert3x1KzbpzMGt2qdqrAThiRovi8");
		_skip_list.insert("015ArtCgi3wmpQAAfYx4riaFmo4prJA4VsK");
		_skip_list.insert("001dice97ECuByXAvqXpaYzSaQuPVvrtmz6");
		_skip_list.insert("001dice9wVtrKZTBbAZqz1XiTmboYyvpD3t");
		_skip_list.insert("001dice9wcMu5hLF4g81u8nioL5mmSHTApw");
		_skip_list.insert("001dice2zdoxQHpGRNaAWiqbK82FQhr4fb5");
		_skip_list.insert("001dice1Qf4Br5EYjj9rnHWqgMVYnQWehYG");
		_skip_list.insert("01DFYJ3i6UcWTg3pCxkeJu6ZC7RJHBD5NNn");
		_skip_list.insert("001dice8EMZmqKvrGE4Qc9bUFf9PX3xaYDp");
		_skip_list.insert("001dice7W2AicHosf5EL3GFDUVga7TgtPFn");
		_skip_list.insert("001dice7fUkz5h4z2wPc1wLMPWgB5mDwKDx");
	}

	/* add_address() notes that new address has been seen by our transaction
	 * processor. We use the last 20 bytes of an address as its 'short
	 * address' since it still has the properties of a 160-bit random value
	 * without the less-high-entropy stuff at the start of the address (and
	 * therefore works better for interpolative search).
	 * It adds the address to the maps _shortaddr and _longaddr,
	 * which are mutually inverse.
	 */
	void add_address(const string& address) {
		assert(address.length() > _shortaddr_len);
		_shortaddr[address] = address.substr(
			address.length() - _shortaddr_len, address.length());
		_longaddr[_shortaddr[address]] = address;
	}

	/* Returns the short form address for a bitcoin address */
	string get_short_address(const string& address) const {
		assert(address.length() > _shortaddr_len);
		assert(_shortaddr.count(address));
		return _shortaddr.at(address);
	}

	/* Returns the full bitcoin address for our shortform */
	string get_long_address(const string& address) const {
		assert(address.length() == _shortaddr_len);
		assert(_longaddr.count(address));
		return _longaddr.at(address);
	}

	/* Outputs a file that contains a list of all the addresses.
	   This is sorted by each address's short-form (i.e., last twenty
	   bytes). The file uses the same directory and filename prefix and
	   attaches "_address_listing".
	 */
	void output_address_manifest() {
		string name = Logger::stringify("%/%_address_listing",
			      _directory, _filename);
		ofstream fout(name);
		assert(fout.good());
		Logger::info("(txproc) write address list: %", name);

		for (auto &x : _addr_to_blocks) {
			fout << get_long_address(x.first) << endl;
			assert(fout.good());
		}
	}

	/* build_address_map(): represents the set @blocks as a binary string of
	 * length @_pir_blocks (in bits) with 1 if that position is in @blocks
	 * and 0 otherwise. It adds this string (prefixed by address) to the
	 * vector @out
	 */
	virtual void build_address_map(const string& address,
				       const set<uint32_t>& blocks,
				       vector<string>* out) const {
		assert(out);
		stringstream ss;
		ss << get_long_address(address);
		int i = 0;
		while (i < _pir_blocks) {
			uint8_t val = 0;
			for (int j = 0; j < 8; ++j, ++i) {
				if (blocks.count(i)) {
					val += 1;
				}
				val <<= 1;
			}
			ss << val;
		}
		out->push_back(ss.str());
	}

	/* build_address_list(): represents the set @blocks as a list of
	 * numbers. It adds this string (prefixed by address) to the
	 * vector @out
	 */
	virtual void build_address_list(const string& address,
				        const set<uint32_t>& blocks,
				        vector<string>* out) const {
		assert(out);
		stringstream ss;
		ss << get_long_address(address);
		uint32_t len = blocks.size();
		ss.write(reinterpret_cast<const char*>(&len),
			 sizeof(len));
		for (auto &x: blocks) {
			ss.write(reinterpret_cast<const char*>(
				 &x), sizeof(x));
		}
		out->push_back(ss.str());
	}

	/* Outputs the first pir database consisting of address-sorted list of
	 * blocks in the main database to be retrieved.
	 */
	void output_address_formats() {
		vector<string> format1, format2;
		vector<string> address_list;

		for (const auto &x : _addr_to_blocks) {
			address_list.push_back(get_long_address(x.first));
			build_address_map(x.first, x.second, &format1);
			build_address_list(x.first, x.second, &format2);
		}
		assert(address_list.size());
		assert(format1.size());
		assert(format2.size());
		assert(address_list.size() == format1.size());
		assert(address_list.size() == format2.size());
		{
			AutoDeliminatedPIRDatabase deliminated_pir_database1(
				_directory, "addr_db.fmt1");
			deliminated_pir_database1.build(address_list, format1);
		}
		{
			DeliminatedPIRDatabase deliminated_pir_database2(
				_directory, "addr_db.fmt2");
			deliminated_pir_database2.build(address_list, format2);
		}
	}

	/* remap_addresses(): this function takes a map from addresses to
	 * positions and produces a map of addresses to the set of PIR blocks
	 * where that transaction is located. Effectively converts byte
	 * positions to block positions for each address's transaction.
	 */
	virtual void remap_addresses() {
		for (auto &x : _addr_to_positions) {
			if (_skip_list.count(x.first)) continue;
			for (auto &y : x.second) {
				for (auto &z : _pos_to_blocks[y]) {
					_addr_to_blocks[x.first].insert(z);
				}
			}
		}
	}

	/* trace(): for each address, tell how many blocks to get. */
	virtual void trace() {
		for (auto &x: _addr_to_blocks) {
			Logger::info("(btpir) addr % \t has % blocks to get.",
				     x.first, x.second.size());
		}
	}

	// Prohibit copy
	TransactionProcessor(const TransactionProcessor& copy) {}

	/* directory where all PIR files are stored. */
	string _directory;

	/* a filename prefix for all PIR database output files. */
	string _filename;

	/* maximum transaction length */
	size_t _max_len;
	uint64_t _db_size;
	uint64_t _pos;

	/* PIR blocks in the main database. */
	uint64_t _pir_blocks;

	/* PIR blocksize for the main database */
	uint64_t _pir_blocksize;

	vector<string> _txs;

	map<string, uint64_t> _addr_to_tx_len;
	map<string, set<uint32_t>> _addr_to_blocks;
	map<string, set<uint32_t>> _addr_to_positions;

	/* Maps a transaction position to the set of PIR blocks
	   where it is stored. Generally the set is going to have one entry, or
	   two if it crosses a PIR block boundary.
	 */
	map<uint64_t, set<uint32_t>> _pos_to_blocks;

	/* a set of addresses whose owners achieve better performance
	   by downloading the whole block chain. */
	set<string> _skip_list;

	map<string, string> _longaddr;  // short to full addr
	map<string, string> _shortaddr; // full to short addr

	/* total bytes used in main transaction database */
	uint64_t _tx_data_sum;

	/* bytes used for a short address */
	size_t _shortaddr_len;

	/* bytes used for a long address */
	size_t _longaddr_len;

	/* bytes used to represent transaction length preceeding transaction */
	size_t _len_len;

	/* current transaction offset in the PIR database */
	uint64_t _pirdb_pos;
};

}  // namespace bitcoin_pir

#endif  // __BTPIR__BUILD_DATABASE__TRANSACTION_PROCESSOR__H__
