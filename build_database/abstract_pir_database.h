/*
   Copyright 2016 Joel Reardon, UC Berkeley

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

#ifndef __BTPIR__ABSTRACT_PIR_DATABASE__H__
#define __BTPIR__ABSTRACT_PIR_DATABASE__H__

#include <fstream>
#include <string>
#include <vector>

#include "ib/logger.h"

using namespace std;
using namespace ib;

namespace btpir {
/* The AbstractPIRDatabase is the pure abstract class for PIR databases.
 * It covers the basic routines to store data, retrieve sizes, and output the
 * data to a file.
 */
class AbstractPIRDatabase {
public:
	virtual ~AbstractPIRDatabase() {}

	virtual void build(const vector<string>& addresses,
			   const vector<string>& data) = 0;
	virtual void open_for_write() = 0;

	/* Called whenever a new transaction is being added to the database.
	 * @address is address it is linked to, length is the length of
	 * the data corresponding to the transaction.
	 */
	virtual void start_tx(const string& address, uint32_t length) = 0;

	/* Called whenever a new transaction is being added to the database.
	 * @address is address it is linked to, length is the length of
	 * the data corresponding to the transaction.
	 */
	virtual void end_tx(const string& address, uint32_t length) = 0;

	/* Returns the total data that has been written to the PIR database.
	 */
	virtual size_t total_written() = 0;
};

}  // namespace bitcoin_pir

#endif  // __DELIMINATED_PIR_DATABASE__H__
