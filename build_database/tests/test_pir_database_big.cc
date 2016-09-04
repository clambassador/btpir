/*
 * =====================================================================================
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
 * =====================================================================================
*/

#include "build_database/transaction_processor.h"

#include <cstdint>
#include <iostream>
#include <fstream>
#include <random>
#include <sstream>
#include <string>

using namespace btpir;
using namespace std;

int main(int argc, char** argv) {
	TransactionProcessor db(".", "test_out.pirdb");
	vector<set<string>> sets;
	vector<string> addrs;
	std::default_random_engine generator;
	string b58chars =
		"123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
	std::uniform_int_distribution<int> distribution(
		0, b58chars.length() - 1);

	for (int i = 0; i < 100000; ++i) {
		if (!(i % 10000)) Logger::info("at %", i);
		set<string> cur_addr;
		stringstream ss;
		for (int j = 0; j < 35; ++j) {
			char c = b58chars[distribution(generator)];
			ss << c;
		}
		addrs.push_back(ss.str());
		cur_addr.insert(ss.str());
		cur_addr.insert(addrs[i % addrs.size()]);
		if (i % 2) cur_addr.insert(addrs[(i * i) % addrs.size()]);
		string s;
		s.resize(150);
		db.add_tx(cur_addr, s);
	}
}
