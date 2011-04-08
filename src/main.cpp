/*****************************************************************************
ArXSum - HashSum generating / checking utility.

Copyright (c) 2006 Alexander 'Elric' Fokin. All Rights Reserved. 
ru.elric@gmail.com
 
This product includes cryptographic software written by
Eric Young (eay@cryptsoft.com)

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose, without fee, and without a written
agreement, is hereby granted, provided that the above copyright notice, 
this paragraph and the following two paragraphs appear in all copies, 
modifications, and distributions.

IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT,
INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST
PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
EVEN IF THE AUTHOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF
ANY, PROVIDED HEREUNDER IS PROVIDED "AS IS". THE AUTHOR HAS NO OBLIGATION
TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

TODO:
- ..\* and * duplicates

v1.2.0
+ Major rewrite using boost and OOP, the first step towards linux port

v1.1.1
+ TTH hash added

v1.1.0
* Fixed eof without eoln issue
* FILE_SHARE_READ added in CreateFile

v1.0.9
* Fixed absolute paths in .md5 files

v1.0.8
+ Simultaneous output into several files with different formats

v1.0.7
* IsValidFileName fixed - there were bugs with characters with code > 127

v1.0.6
+ Multithreading added

v1.0.5
* Changed command line options format
+ Added .ed2k file checking
+ Added .sfv file output

v1.0.4
* Now treating lines in .md5 files starting with '#' as comments
  Elric: this may lead to a bug in parsing .sfv files
* Fixed bug with blank lines in .md5 files
* IsValidFileName fixed
+ Added .ed2k file output

v1.0.3
* ed2k hashing method changed to the one used in eMule 0.46c
  see http://wiki.anidb.info/w/AniDB:Ed2k-hash for details

v1.0.2
+ Overlapped IO

v1.0.1
+ Output hashing speed
+ Now using faster IO routines (~ x2 speedup)
+ Checksum verification
+ Read filelist from file / stdin

v1.0.0
+ First Release - crc, md4, ed2k, md5, sha1, sha256, sha512 hashing
*****************************************************************************/

#include "config.h"
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <set>
#include <vector>
#include <algorithm>
#include <time.h>
#include <boost/foreach.hpp>
#include <boost/array.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/timer.hpp>
#include "Hash.h"
#include "Collections.h"
#include "FileEntry.h"
#include "Hasher.h"
#include "Task.h"
#include "Output.h"
#include "Streams.h"
#include "Checking.h"
#include "Synopsis.h"

#include <tchar.h>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace boost::filesystem;

void testHashSpeed() {
	cout << "Testing hash speed..." << endl;
	unsigned char* dataChunk = new unsigned char[TESTDATACHUNKSIZE];
	for(int i = 0; i < H_COUNT; i++) {
		Hash hash(i, (uint64) TESTDATACHUNKSIZE * TESTMAXITERATIONS);
		cout << "  " << hash.getName() << ": ";
		timer t;
		int iterations = 0;
		while(t.elapsed() < TESTTIME) {
			hash.update(dataChunk, TESTDATACHUNKSIZE);
			if(++iterations > TESTMAXITERATIONS)
				break;
		}
		cout << fixed << showpoint << setprecision(3) << 
			(1.0 * iterations * TESTDATACHUNKSIZE) / (t.elapsed() * 1024 * 1024) << "Mbps" << endl;
	}
	cout << "End of test." << endl;
	delete[] dataChunk;
}

ArrayList<FileEntry> addToFileList(path dir, regex fileNameMask, bool isRecursive, ArrayList<FileEntry> fileList) {
	directory_iterator end;
	try {
		for(directory_iterator itr(dir); itr != end; itr++) {
			if(isRecursive && is_directory(itr->status())) {
				addToFileList(*itr, fileNameMask, isRecursive, fileList);
			} else if(is_regular(itr->status())) {
				if(regex_match(itr->leaf(), fileNameMask)) {
					string filePath = (dir / itr->leaf()).external_file_string();
					if(starts_with(filePath, "."))
						filePath = filePath.substr(2);
					fileList.add(filePath);
				}
			}
		}
	} catch (exception) {
		// cout << e.what() << endl;
	}
	return fileList;
}

ArrayList<FileEntry> addToFileList(string filePathMask, bool isRecursive, ArrayList<FileEntry> fileList) {
	path fullPath(filePathMask);
	path dir = fullPath.branch_path();
	if(dir.empty())
		dir = path(".");
	string mask = fullPath.leaf();
	struct regex_replacement {
		regex expression;
		string replacement;
	};
	const static regex_replacement replaces[] = {
		//{regex("\\\\"), "\\\\\\\\"},
		{regex("\\."), "\\\\."},
		{regex("\\["), "\\\\["},
		{regex("\\{"), "\\\\{"},
		{regex("\\("), "\\\\("},
		{regex("\\)"), "\\\\)"},
		{regex("\\+"), "\\\\+"},
		//{regex("\\|"), "\\\\|"},
		{regex("\\^"), "\\\\^"},
		{regex("\\$"), "\\\\$"},
		{regex("\\?"), "."},
		{regex("\\*"), ".*"}
	};
	for(int i = 0; i < sizeof(replaces) / sizeof(regex_replacement); i++)
		mask = regex_replace(mask, replaces[i].expression, replaces[i].replacement);
	return addToFileList(dir, regex(mask), isRecursive, fileList);
}

bool recursive = false;
bool multiThreaded = false;
bool checkMode = false;
bool quiet = false;
bool useFileList = false;
HashTask hashTask;
OutputTask outTask;
ArrayList<string> fileMasks;
ArrayList<FileEntry> fileList;

void processCommandLine(int argc, char** argv) {
	for(int i = 1; i < argc; i++) {
    string arg(argv[i]);
    if(starts_with(arg, "-")) {
      if(arg == "-h" || arg == "--help") {
        synopsis();
        exit(0);
      }
			else if(arg == "-t" || arg == "--test") {
        testHashSpeed();
				exit(0);
			}
      else if(arg == "-r")
        recursive = true;
      else if(arg == "-m")
        multiThreaded = true;
      else if(arg == "-c")
        checkMode = true;
      else if(arg == "-q")
        quiet = true;
			else if(arg == "-l")
        useFileList = true;
      else if(arg == "-all") {
        for(int i = 0; i < H_COUNT; i++)
					hashTask.add(i);
			} else if(arg == "-crc")
				hashTask.add(H_CRC);
      else if(arg == "-md4")
				hashTask.add(H_MD4);
      else if(arg == "-ed2k")
				hashTask.add(H_ED2K);
      else if(arg == "-md5")
				hashTask.add(H_MD5);
      else if(arg == "-sha1")
				hashTask.add(H_SHA1);
      else if(arg == "-sha256")
				hashTask.add(H_SHA256);
      else if(arg == "-sha512")
				hashTask.add(H_SHA512);
      else if(arg == "-tth")
				hashTask.add(H_TTH);
			else if(starts_with(arg, "-o")) {
				if(++i >= argc)
					synopsisAndExit();
				string fileName = argv[i];
				if(arg == "-omd5")
					outTask.add(O_MD5, fileName);
				else if(arg == "-osfv")
					outTask.add(O_SFV, fileName);
				else if(arg == "-oed2k")
					outTask.add(O_ED2K, fileName);
				else
					synopsisAndExit();
			} else
				synopsisAndExit();
    }
    else
      fileMasks.add(argv[i]);
  }

	// If nothing specified - calculate md5
	uint32 sum = 0;
	for(uint32 i = 0; i < outTask.size(); i++)
		sum += outTask.isSet(i) ? 1 : 0;
	if(sum == 0)
		outTask.add(O_MD5, "-");

	// Calculate hashes required by output format
	for(uint32 i = 0; i < outTask.size(); i++)
		if(outTask.isSet(i))
			hashTask.add(outTaskRequirement[i]);
}

int main(int argc, char **argv) {
	processCommandLine(argc, argv);

	FOREACH(string fileMask, fileMasks)
		addToFileList(fileMask, recursive, fileList);

	if(fileMasks.empty())
    fileList.add("-");

	if(fileList.empty())
		synopsisAndExit();

	if(useFileList) {
		ArrayList<FileEntry> newFileList;
		FOREACH(FileEntry file, fileList) {
			InputStream stream = createInputStream(file.getPath());
			if(stream->fail()) {
				cout << "[error] cannot access file: " << file.getPath() << endl;
				continue;
			}
			while(!stream->eof() && !stream->fail()) {
				string fileName;
				getline(*stream.get(), fileName);
				newFileList.add(fileName);
			}
		}
		if(newFileList.empty())
	    newFileList.add("-");
		fileList = newFileList;
	}

	sort(fileList.begin(), fileList.end());

	uint64 sumSize = 0;
	FOREACH(FileEntry file, fileList) {
		if(file.getPath() == "-") {
			file.setSize(numeric_limits<uint64>::max());
			time_t lTime;
			time(&lTime);
			file.setDateTime(lTime);
			sumSize += file.getSize();
		} else {
			try {
				file.setSize(file_size(file.getPath()));
				sumSize += file.getSize();
				file.setDateTime(last_write_time(file.getPath()));
			} catch (exception e) {
				file.setFailed(true);
				cout << "[error] cannot access file: " << file.getPath() << endl;
			}
		}
	}

	if(checkMode) {
		FOREACH(FileEntry file, fileList)
			check(file.getPath());
		return 0;
	}

	shared_ptr<StreamOutCallBack> pCallback;
	if(!quiet)
		pCallback.reset(new StreamOutCallBack(sumSize, createWriter("-")));
	Hasher hasher(hashTask, multiThreaded, pCallback);
	FOREACH(FileEntry file, fileList) 
		if(!file.isFailed())
			hasher.hash(file);
	
	outTask.perform(hashTask, fileList);
	
  return 0;
}


