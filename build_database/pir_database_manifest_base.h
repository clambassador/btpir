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

#ifndef __BTPIR__BUILD_DATABASE__PIR_DATABASE_MANIFEST_BASE__H__
#define __BTPIR__BUILD_DATABASE__PIR_DATABASE_MANIFEST_BASE__H__

#include "build_database/pir_database_base.h"

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
class PIRDatabaseManifestBase : public PIRDatabaseBase {
public:
	PIRDatabaseManifestBase(const string& directory,
				const string& filename)
			: PIRDatabaseBase(directory, filename) {
		_fmanifest.reset(new ofstream(_filename + ".pir.manifest"));

	}
	/* Destructor finishes writing the database. It fills the final block's
	 * leftover content with zeros and closes the file.
	 */
	virtual ~PIRDatabaseManifestBase() {
		/* rename files to have useful data handy */
		_fmanifest->close();
                string old_filename = Logger::stringify("%.pir.manifest",
                                                        _filename);
                string new_filename = Logger::stringify("%_%_%.pir.manifest",
                                                        _filename,
                                                        _blocks,
                                                        _pir_blocksize_bytes);
                assert(!rename(old_filename.c_str(),
                               new_filename.c_str()));
	}

protected:
	/* new_block(): called whenever a new PIR block is created. */
	virtual void new_block(size_t remaining) {
		*_fmanifest << _cur_addr << endl;
		assert(_fmanifest->good());
		PIRDatabaseBase::new_block(remaining);
	}

	unique_ptr<ofstream> _fmanifest;
};

}  // namespace btpir

#endif  // __PIR_DATABASE_MANIFEST_BASE__H__
