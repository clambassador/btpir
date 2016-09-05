#include "build_database/transaction_processor.h"

#include <cassert>
#include <fstream>

#include "ib/logger.h"

using namespace std;
using namespace ib;
using namespace btpir;

int main(int argc, char **argv) {
	if (argc != 4) {
		Logger::error("usage: % tx_file output_directory "
			      "output_file_prefix", argv[0]);
		Logger::error("");
		Logger::error("");
		Logger::error("---------------------------------------");
		Logger::error("FORMAT OF TX_FILE  (including newlines)");
		Logger::error("---------------------------------------");
		Logger::error("<TX1 number of addresses>");
		Logger::error("addr1");
		Logger::error("addr2");
		Logger::error("...");
		Logger::error("<length of transaction TX1>");
		Logger::error("<TX1 binary string of transaction length>");
		Logger::error("<TX2 number of addresses>");
		Logger::error("addr1");
		Logger::error("addr2");
		Logger::error("...");
		Logger::error("<length of transaction TX2>");
		Logger::error("<TX2 binary string of transaction length>");
		Logger::error("...");
		Logger::error("---------------------------------------");
		return -1;
	}
	string tx_file = argv[1];
	string directory = argv[2];
	string filename = argv[3];
	vector<set<string>> addresses;
	vector<string> transactions;

	{
		ifstream fin(tx_file);
		assert(fin.good());

		while (fin.good()) {
			size_t number_addresses;
			size_t transaction_length;
			fin >> number_addresses;
			if (!fin.good()) break;
			addresses.push_back(set<string>());
			for (size_t i = 0; i < number_addresses; ++i) {
				string address;
				fin >> address;
				assert(fin.good());
				assert(!addresses.back().count(address));
				addresses.back().insert(address);
			}
			string dummy;
			fin >> transaction_length;
			getline(fin, dummy);
			assert(fin.good());
			assert(dummy.empty());
			unique_ptr<char[]> buf(new char[transaction_length]);
			fin.read(buf.get(), transaction_length);
			assert(fin.good());
			getline(fin, dummy);
			assert(fin.good());

			assert(dummy.empty());
			transactions.push_back("");
			transactions.back().assign(buf.get(),
						   transaction_length);

		}
	}
	assert(addresses.size() == transactions.size());

	TransactionProcessor processor(directory, filename);
	for (size_t i = 0; i < addresses.size(); ++i) {
		processor.add_tx(addresses[i], transactions[i]);
	}

	return 0;
}
