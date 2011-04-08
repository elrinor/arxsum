#include "config.h"
#include "Checker.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include "arx/Exception.h"
#include "arx/Collections.h"
#include "arx/Converter.h"
#include "Torrent.h"
#include "Output.h"
#include "Hasher.h"
#include "Options.h"
#include "Streams.h"

using namespace std;
using namespace arx;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::algorithm;

// -------------------------------------------------------------------------- //
// CheckError
// -------------------------------------------------------------------------- //
class CheckError::CheckErrorImpl {
public:
  CheckErrorType type;
  int hashId;
  Digest rightHash;
  Digest wrongHash;
  uint64 rightSize;
  uint64 wrongSize;
  uint32 lineNumber;

  CheckErrorImpl(CheckErrorType type) {
    this->type = type;
  }

  void throwIfNotWrongHash() {
    if(this->type != CE_WRONGHASH)
      throw runtime_error("CheckError isn't of type CE_WRONGHASH");
  }

  void throwIfNotWrongSize() {
    if(this->type != CE_WRONGSIZE)
      throw runtime_error("CheckError isn't of type CE_WRONGSIZE");
  }

  void throwIfNotWrongFormat() {
    if(this->type != CE_WRONGFORMAT)
      throw runtime_error("CheckError isn't of type CE_WRONGFORMAT");
  }
};

CheckErrorType CheckError::getType() {
  return this->impl->type;
}

int CheckError::getHashId() {
  this->impl->throwIfNotWrongHash();
  return this->impl->hashId;
}

Digest CheckError::getRightHash() {
  this->impl->throwIfNotWrongHash();
  return this->impl->rightHash;
}

Digest CheckError::getWrongHash() {
  this->impl->throwIfNotWrongHash();
  return this->impl->wrongHash;
}

uint64 CheckError::getRightSize() {
  this->impl->throwIfNotWrongSize();
  return this->impl->rightSize;
}

uint64 CheckError::getWrongSize() {
  this->impl->throwIfNotWrongSize();
  return this->impl->wrongSize;
}

uint32 CheckError::getLineNumber() {
  this->impl->throwIfNotWrongFormat();
  return this->impl->lineNumber;
}

CheckError::CheckError(CheckErrorType type): impl(new CheckErrorImpl(type)) {
  return;
}

CheckError::CheckError(CheckErrorType type, uint32 lineNumber): impl(new CheckErrorImpl(type)) {
  this->impl->lineNumber = lineNumber;
}

CheckError::CheckError(CheckErrorType type, uint64 rightSize, uint64 wrongSize): impl(new CheckErrorImpl(type)) {
  this->impl->throwIfNotWrongSize();
  this->impl->rightSize = rightSize;
  this->impl->wrongSize = wrongSize;
}

CheckError::CheckError(CheckErrorType type, int hashId, Digest rightHash, Digest wrongHash): impl(new CheckErrorImpl(type)) {
  this->impl->throwIfNotWrongHash();
  this->impl->hashId = hashId;
  this->impl->rightHash = rightHash;
  this->impl->wrongHash = wrongHash;
}

// -------------------------------------------------------------------------- //
// CheckResultReporter
// -------------------------------------------------------------------------- //
void CheckResultReporter::error(CheckError error) {
  return;
}

void CheckResultReporter::error(std::wstring errorString) {
  return;
}

void CheckResultReporter::begin(const boost::filesystem::wpath& checkSumFilePath) {
  return;
}

void CheckResultReporter::beginFile(const boost::filesystem::wpath& filePath, const std::wstring& fileString) {
  return;
}

void CheckResultReporter::update(uint64 justProcessed) {
  return;
}

void CheckResultReporter::endFile(arx::ArrayList<CheckError> errors) {
  return;
}

void CheckResultReporter::end() {
  return;
}


// -------------------------------------------------------------------------- //
// PrinterCheckResultReporter
// -------------------------------------------------------------------------- //
PrinterCheckResultReporter::PrinterCheckResultReporter(arx::Printer* printer): printer(printer) {
  this->progressCallBack = StreamOutHasherCallBack((uint64) -1, this->printer);
}

void PrinterCheckResultReporter::error(CheckError error) {
  this->errorCount++;
  if(error.getType() == CE_NOACCESS)
    *this->printer << "[error] could not open file: " << this->checkSumFileName << endl;
  else if(error.getType() == CE_WRONGFORMAT)
    *this->printer << "[error] unreadable file format (line " << error.getLineNumber() << ")" << endl;
  else if(error.getType() == CE_TOOMANYERRORS)
    *this->printer << "[error] too many errors, giving up on " << this->checkSumFileName << endl;
}

void PrinterCheckResultReporter::error(std::wstring errorString) {
  this->errorCount++;
  *this->printer << "[error]" << errorString << endl;
}

void PrinterCheckResultReporter::begin(const wpath& checkSumFilePath) {
  *this->printer << "  " << checkSumFilePath << ": " << endl;
  this->checkSumFileName = checkSumFilePath.native_file_string();
  this->errorCount = 0;
  this->okCount = 0;
}

void PrinterCheckResultReporter::beginFile(const wpath& filePath, const wstring& fileString) {
  this->progressCallBack = StreamOutHasherCallBack((uint64) -1, this->printer);
  this->fileName = fileString;
  try {
    this->fileSize = file_size(filePath);
    this->progressCallBack = StreamOutHasherCallBack(this->fileSize, this->printer);
  } catch (...) {
    return;
  }
}

void PrinterCheckResultReporter::update(uint64 justProcessed) {
  this->progressCallBack(justProcessed);
}

void PrinterCheckResultReporter::endFile(ArrayList<CheckError> errors) {
  this->progressCallBack.clear();
  if(errors.empty()) {
    *this->printer << "[ok] " << this->fileName << endl;
    this->okCount++;
  } else FOREACH(CheckError error, errors) {
    this->errorCount++;
    if(error.getType() == CE_WRONGHASH) {
      *this->printer << "[error] wrong " << Hash::getName(error.getHashId()) << ": " << this->fileName << endl;
    } else if(error.getType() == CE_WRONGSIZE) {
      *this->printer << "[error] wrong file size (" << error.getRightSize() << " != " << error.getWrongSize() << "): " << this->fileName << endl;
    } else if(error.getType() == CE_NOACCESS) {
      *this->printer << "[error] could not open file: " << this->fileName << endl;
    }
  }
}

void PrinterCheckResultReporter::end() {
  wCout << "  Errors: " << this->errorCount << endl;
  wCout << "  Ok: " << this->okCount << endl;
  wCout << endl;
}


// -------------------------------------------------------------------------- //
// DelegatingHasherCallBack
// -------------------------------------------------------------------------- //
class DelegatingHasherCallBack: public HasherCallBack {
private:
  boost::shared_ptr<CheckResultReporter> reporter;
public:
  DelegatingHasherCallBack(boost::shared_ptr<CheckResultReporter> reporter): reporter(reporter) {
    return;
  }
  void operator() (uint64 justProcessed) {
    this->reporter->update(justProcessed);
  }
  void clear() {
    return;
  }
};


// -------------------------------------------------------------------------- //
// CheckerImpl
// -------------------------------------------------------------------------- //
class Checker::CheckerImpl {
private:
  boost::shared_ptr<CheckResultReporter> reporter;

protected:
  void check(boost::filesystem::wpath checkSumFilePath, int guessedFormat) {
    wpath dir = checkSumFilePath.branch_path();
    if(dir == _T(""))
      dir = _T(".");

    Reader reader;
    try {
      reader = createReader(checkSumFilePath, options.getInputEncoding(), false);
    } catch (...) {
      this->reporter->error(CheckError(CE_NOACCESS));
    }
    Scanner scanner(reader);

    this->reporter->begin(checkSumFilePath);

    int lineN = 0;
    int criticalErrorCount = 0;
    while(!scanner.eof()) {
      wstring s;
      getline(scanner, s);
      trim(s);
      lineN++;

      if(s.size() == 0)
        continue;

      if(s[0] == _T(';') || s[0] == _T('#')) // TODO: in *.sfv # may be not a comment
        continue;

      if(criticalErrorCount > HASHFILEMAXERRORS) {
        this->reporter->error(CheckError(CE_TOOMANYERRORS));
        break;
      }

      uint32 hashId = H_UNKNOWN;
      wstring fileName;
      wstring digest;
      uint64 fileSize;

      const static wstring fileNameRegex = _T("([^\"\\*\\?<>\\|]+)");
      const static wregex regexEd2k(_T("ed2k://\\|file\\|") + fileNameRegex + _T("\\|([0-9]+)\\|([0-9a-fA-F]{32})\\|/"));
      const static wregex regexMd5(_T("([0-9a-fA-F]{32})[ \t\\*]+") + fileNameRegex);
      const static wregex regexSha1(_T("([0-9a-fA-F]{40})[ \t\\*]+") + fileNameRegex);
      const static wregex regexCrc(fileNameRegex + _T("[ \t]+([0-9a-fA-F]{8})"));
      const static wregex regexBsd(_T("([a-zA-Z]+) *\\(") + fileNameRegex + _T("\\) *= *([0-9a-fA-F]+)"));

      wsmatch matchEd2k;
      wsmatch matchMd5;
      wsmatch matchCrc;
      wsmatch matchSha1;
      wsmatch matchBsd;
      if(regex_match(s, matchEd2k, regexEd2k)) {
        hashId = H_ED2K;
        fileName = wstring(matchEd2k[1].first, matchEd2k[1].second);
        fileSize = lexical_cast<uint64>(string(matchEd2k[2].first, matchEd2k[2].second));
        digest = wstring(matchEd2k[3].first, matchEd2k[3].second);
      } else {
        regex_match(s, matchMd5, regexMd5);
        regex_match(s, matchCrc, regexCrc);
        regex_match(s, matchSha1, regexSha1);
        if((matchMd5[0].matched || matchSha1[0].matched) && matchCrc[0].matched && (guessedFormat != O_MD5 && guessedFormat != O_SFV && guessedFormat != O_SHA1)) {
          this->reporter->error(CheckError(CE_WRONGFORMAT, lineN));
          criticalErrorCount++;
          continue;
        } else if(matchMd5[0].matched && guessedFormat == O_MD5)
          hashId = H_MD5;
        else if(matchCrc[0].matched && guessedFormat == O_SFV)
          hashId = H_CRC;
        else if(matchSha1[0].matched && guessedFormat == O_SHA1)
          hashId = H_SHA1;
        else if(matchMd5[0].matched)
          hashId = H_MD5;
        else if(matchCrc[0].matched)
          hashId = H_CRC;
        else if(matchSha1[0].matched)
          hashId = H_SHA1;
        if(hashId == H_MD5) {
          fileName = wstring(matchMd5[2].first, matchMd5[2].second);
          digest = wstring(matchMd5[1].first, matchMd5[1].second);
        } else if(hashId == H_SHA1) {
          fileName = wstring(matchSha1[2].first, matchSha1[2].second);
          digest = wstring(matchSha1[1].first, matchSha1[1].second);
        } else if(hashId == H_CRC) {
          fileName = wstring(matchCrc[1].first, matchCrc[1].second);
          digest = wstring(matchCrc[2].first, matchCrc[2].second);
        }
      }

      if(hashId == H_UNKNOWN) {
        this->reporter->error(CheckError(CE_WRONGFORMAT, lineN));
        criticalErrorCount++;
        continue;
      }

      wpath filePath = wpath(fileName).is_complete() ? fileName : (dir / fileName);

      this->reporter->beginFile(filePath, fileName);
      ArrayList<CheckError> errors;
      try {
        uint64 actualFileSize = file_size(dir / fileName);
        shared_ptr<DelegatingHasherCallBack> callBack(new DelegatingHasherCallBack(this->reporter));
        Digest realDigest = Hasher::hash(hashId, filePath, callBack);
        Digest rightDigest = Digest::fromHexString(toNarrowString(digest));
        if(realDigest != rightDigest)
          errors.push_back(CheckError(CE_WRONGHASH, hashId, rightDigest, realDigest));
        if(hashId == H_ED2K && actualFileSize != fileSize)
          errors.push_back(CheckError(CE_WRONGSIZE, fileSize, actualFileSize));
      } catch (exception) {
        errors.push_back(CheckError(CE_NOACCESS));
      }
      this->reporter->endFile(errors);
    }

    this->reporter->end();
    }

public:
  CheckerImpl(boost::shared_ptr<CheckResultReporter> reporter): reporter(reporter) {
    return;
  }

  void check(boost::filesystem::wpath paramFilePath) {
    wstring fileName = to_lower_copy(paramFilePath.leaf());
    
    if(ends_with(fileName, _T(".torrent"))) {
      checkTorrent(paramFilePath, this->reporter);
    } else {
      uint32 guess = H_UNKNOWN;
      uint32 recognizedCount = 0;
      if(contains(fileName, _T("crc")) || contains(fileName, _T("sfv"))) {
        recognizedCount++;
        guess = O_SFV;
      }
      if(contains(fileName, _T("md5"))) {
        recognizedCount++;
        guess = O_MD5;
      }
      if(contains(fileName, _T("ed2k"))) {
        recognizedCount++;
        guess = O_ED2K;
      }
      if(contains(fileName, _T("sha1"))) {
        recognizedCount++;
        guess = O_SHA1;
      }
      if(contains(fileName, _T("bsd"))) {
        recognizedCount++;
        guess = O_BSD;
      }
      if(recognizedCount > 1)
        guess = H_UNKNOWN;
      check(paramFilePath, guess);
    }
  }
};


// -------------------------------------------------------------------------- //
// Checker
// -------------------------------------------------------------------------- //
Checker::Checker(boost::shared_ptr<CheckResultReporter> reporter): impl(new CheckerImpl(reporter)) {
  return;
}

void Checker::check(boost::filesystem::wpath filePath)  {
  this->impl->check(filePath);
}

