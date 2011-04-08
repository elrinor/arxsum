#include "config.h"
#include "Checking.h"
#include "Hash.h"
#include "Hasher.h"
#include "Streams.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::algorithm;

void check(path hashFilePath, uint32 guessedHashId) {
	path dir = hashFilePath.branch_path();
	if(dir == "")
		dir = ".";
	Reader reader = createReader(hashFilePath);
	if(reader->fail()) {
		cout << "[error] cannot access file: " << hashFilePath << endl << endl;
		return;
	}
	cout << "  " << hashFilePath << ": " << endl;
	int lineN = 0;
	int criticalErrorCount = 0;
	int errorCount = 0;
	int wrongHashCount = 0;
	int okCount = 0;
	while(!reader->eof()) {
		string s;
		getline(*reader.get(), s);
		trim(s);
		lineN++;

		if(s.size() == 0)
			continue;

		if(s[0] == ';' || s[0] == '#') // TODO: in *.sfv # may be not a comment
			continue;

		if(criticalErrorCount > HASHFILEMAXERRORS) {
			cout << "[error] too many errors, giving up on " << hashFilePath << endl;
			break;
		}

		uint32 hashId = H_UNKNOWN;
		string fileName;
		string digest;
		uint64 fileSize; // for ed2k

		const static regex regexEd2k("ed2k://\\|file\\|([^\"\\*\\?<>\\|\\\\]+)\\|([0-9]+)\\|([0-9a-fA-F]{32})\\|/");
		const static regex regexMd5("([0-9a-fA-F]{32})[ \t\\*]+([^\"\\*\\?<>\\|\\\\]+)");
		const static regex regexCrc("([^\"\\*\\?<>\\|\\\\]+)[ \t]+([0-9a-fA-F]{32})");

		smatch matchEd2k;
		smatch matchMd5;
		smatch matchCrc;
		if(regex_match(s, matchEd2k, regexEd2k)) {
			hashId = H_ED2K;
			fileName = string(matchEd2k[1].first, matchEd2k[1].second);
			fileSize = lexical_cast<uint64>(string(matchEd2k[2].first, matchEd2k[2].second));
			digest = string(matchEd2k[3].first, matchEd2k[3].second);
		} else {
			regex_match(s, matchMd5, regexMd5);
			regex_match(s, matchCrc, regexCrc);
			if(matchMd5[0].matched && matchCrc[0].matched && (guessedHashId != H_MD5 && guessedHashId != H_CRC)) {
				cout << "[error] ambiguity (line " << lineN << ")" << endl;
				criticalErrorCount++;
				continue;
			} else if(matchMd5[0].matched && matchCrc[0].matched && (guessedHashId == H_MD5 || guessedHashId == H_CRC))
				hashId = guessedHashId;
			else if(matchMd5[0].matched)
				hashId = H_MD5;
			else if(matchCrc[0].matched)
				hashId = H_CRC;
			if(hashId == H_MD5) {
				fileName = string(matchMd5[2].first, matchMd5[2].second);
				digest = string(matchMd5[1].first, matchMd5[1].second);
			} else if(hashId == H_CRC) {
				fileName = string(matchCrc[1].first, matchCrc[1].second);
				digest = string(matchCrc[2].first, matchCrc[2].second);
			}
		}

		if(hashId == H_UNKNOWN) {
			cout << "[error] unreadable file format (line " << lineN << ")" << endl;
			criticalErrorCount++;
			continue;
		}

		try {
			uint64 actualFileSize = file_size(dir / fileName);
			// TODO: quiet!
			shared_ptr<StreamOutCallBack> callBack(new StreamOutCallBack(actualFileSize, createWriter("-")));
			bool digestMatches = (Hasher::hash(hashId, dir / fileName, callBack) == digest);
			callBack->clear();
			if(digestMatches) {
				cout << "[ok] " << fileName << endl;;
				okCount++;
				if(hashId == H_ED2K && actualFileSize != fileSize)
					cout << "[warning] wrong file size (" << fileSize << " != " << actualFileSize << "): " << fileName << endl;
			} else {
				cout << "[error] wrong " << Hash::getName(hashId) << ": " << fileName << endl;
				wrongHashCount++;
			}
		} catch (exception) {
			cout << "[error] could not open file: " << fileName << endl;
			errorCount++;
		}
	}

	cout << "  Errors: " << errorCount << endl;
	cout << "  Ok: " << okCount << endl;
	cout << endl;
}

void check(path hashFilePath) {
	string fileName = to_lower_copy(hashFilePath.leaf());
	uint32 guess = H_UNKNOWN;
	uint32 recognizedCount = 0;
	if(contains(fileName, "crc") || contains(fileName, "sfv")) {
		recognizedCount++;
		guess = H_CRC;
	}
	if(contains(fileName, "md5")) {
		recognizedCount++;
		guess = H_MD5;
	}
	if(contains(fileName, "ed2k")) {
		recognizedCount++;
		guess = H_ED2K;
	}
	if(recognizedCount > 1)
		guess = H_UNKNOWN;

	check(hashFilePath, guess);
}


