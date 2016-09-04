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
	sets[0].insert("b______________________r___________");
	sets[1].insert("a______________________z___________");
	sets[1].insert("b______________________r___________");
	sets[1].insert("c______________________a___________");
	sets[2].insert("d______________________j___________");
	sets[3].insert("e______________________e___________");
	sets[4].insert("f______________________o___________");
	sets[5].insert("g_____________________j____________");
	sets[6].insert("h_____________________h____________");
	sets[6].insert("i_____________________n____________");
	sets[3].insert("f_____________________m____________");
	sets[4].insert("g_____________________j____________");
	sets[5].insert("h_____________________h____________");
	sets[6].insert("i_____________________n____________");
	sets[6].insert("a______________________z___________");
	vector<string> words;
	words.push_back("swerve");
	words.push_back("shore");
	words.push_back("bend");
	words.push_back("bay");
	words.push_back("brings");
	words.push_back("recirculation");
	words.push_back("back");

	ofstream fout("test_tx_list");
	for (int i = 0; i < words.size(); ++i) {
		words[i].resize(800 * i);
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
