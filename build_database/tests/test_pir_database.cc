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
#include <string>

using namespace btpir;
using namespace std;

int main(int argc, char** argv) {
	TransactionProcessor db(".", "test_out.pirdb");
	vector<set<string>> sets;
	sets.resize(7);
	sets[0].insert("nwetweryertyer11342ffwerwerf32rdfsd");
	sets[1].insert("adfgdfhmnghkjh12kuyi7ujyrJRHSEDTRHD");
	sets[1].insert("bsgncnnvnxcfgs13HSERWTFJK34rwer346r");
	sets[1].insert("csdfsdr4r43wfg14KJYUFGHFBEDSzdfsdt4");
	sets[2].insert("ddhfkyioktygjf15wqdqasaf3223r23rfas");
	sets[3].insert("edfgdfGDFGASSD168756754gergdfHDFGH4");
	sets[4].insert("f456436eryhDFG171234dBdfgWStJUYIffg");
	sets[5].insert("g54635467355tr18regertgerBREgreafkj");
	sets[6].insert("hfgdhxxcvdfvwe19mmnhfgnxfNDFHBDasff");
	sets[6].insert("iOOLOIGHJDFGSFDa5673643643634trgdfg");
	sets[3].insert("fsdfsGSSjhgOLUYbSCXWESVSerw3rfweyt5");
	sets[4].insert("g346345754erhedcVERberbRTYEHHHKUJ56");
	sets[5].insert("hFHHDFGMKOUJYJRdrwetet3325345DFGDFF");
	sets[6].insert("igertergregggsgA5634534643tyrefgheg");
	sets[6].insert("a4t34tg43grfedgB4534fdgkujk5u654yw4");
	vector<string> words;
	words.push_back("swerve");
	words.push_back("shore");
	words.push_back("bend");
	words.push_back("bay");
	words.push_back("brings");
	words.push_back("recirculation");
	words.push_back("back");

	ofstream fout("test_tx_list");
	for (int j = 0; j < 1000; ++j) {
	for (int i = 0; i < words.size(); ++i) {
		words[i].resize(100 * (i + 1));
		db.add_tx(sets[i], words[i]);
		fout << sets[i].size() << endl;
		for (auto &x : sets[i]) {
			fout << x << endl;
		}
		fout << words[i].length() << endl;
		fout.write(words[i].c_str(), words[i].length());
		fout << endl;
	}
	}
}
