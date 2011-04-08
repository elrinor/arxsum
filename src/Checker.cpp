#include "config.h"
#include "Checker.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_array.hpp>
#include "arx/Exception.h"
#include "arx/Converter.h"
#include "Torrent.h"
#include "ArrayList.h"
#include "Output.h"
#include "Hash.h"
#include "Hasher.h"
#include "Options.h"
#include "Streams.h"
#include "Map.h"

using namespace std;
using namespace arx;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::algorithm;

const wstring fileNameRegex = _T("([^\"\\*\\?<>\\|]+)");

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
namespace detail {
  class CheckResultReporterImpl {
  public:
    virtual void error(CheckError error) {return;};
    virtual void error(std::wstring errorString) {return;};
    virtual void begin(const boost::filesystem::path& checkSumFilePath) {return;};
    virtual void beginFile(const boost::filesystem::path& filePath, const std::wstring& fileString) {return;};
    virtual void update(uint64 justProcessed) {return;};
    virtual void endFile(ArrayList<CheckError> errors) {return;};
    virtual void end() {return;};
  };
}

CheckResultReporter::CheckResultReporter(::detail::CheckResultReporterImpl* impl): impl(impl) {
  return;
}

void CheckResultReporter::error(CheckError error) {
  this->impl->error(error);
}

void CheckResultReporter::error(std::wstring errorString) {
  this->impl->error(errorString);
}

void CheckResultReporter::begin(const boost::filesystem::path& checkSumFilePath) {
  this->impl->begin(checkSumFilePath);
}

void CheckResultReporter::beginFile(const boost::filesystem::path& filePath, const std::wstring& fileString) {
  this->impl->beginFile(filePath, fileString);
}

void CheckResultReporter::update(uint64 justProcessed) {
  this->impl->update(justProcessed);
}

void CheckResultReporter::endFile(ArrayList<CheckError> errors) {
  this->impl->endFile(errors);
}

void CheckResultReporter::end() {
  this->impl->end();
}

CheckResultReporter::CheckResultReporter(): impl(new ::detail::CheckResultReporterImpl()) {
  return; 
}

// -------------------------------------------------------------------------- //
// PrinterCheckResultReporter
// -------------------------------------------------------------------------- //
namespace detail {
  class PrinterCheckResultReporterImpl: public CheckResultReporterImpl {
  private:
    arx::Printer* printer;
    std::wstring checkSumFileName;
    std::wstring fileName;
    uint64 fileSize;
    uint32 okCount, errorCount;
    PrinterHasherCallBack progressCallBack;

  public:
    virtual void error(CheckError error) {
      this->errorCount++;
      if(error.getType() == CE_NOACCESS)
        *this->printer << "[error] could not open file: " << this->checkSumFileName << endl;
      else if(error.getType() == CE_WRONGFORMAT)
        *this->printer << "[error] unreadable file format (line " << error.getLineNumber() << ")" << endl;
      else if(error.getType() == CE_TOOMANYERRORS)
        *this->printer << "[error] too many errors, giving up on " << this->checkSumFileName << endl;
    }

    virtual void error(std::wstring errorString) {
      this->errorCount++;
      *this->printer << "[error]" << errorString << endl;
    }


    virtual void begin(const path& checkSumFilePath) {
      *this->printer << "  " << checkSumFilePath << ": " << endl;
      this->checkSumFileName = checkSumFilePath.native();
      this->errorCount = 0;
      this->okCount = 0;
    }

    virtual void beginFile(const path& filePath, const wstring& fileString) {
      this->progressCallBack = PrinterHasherCallBack((uint64) -1, this->printer);
      this->fileName = fileString;
      try {
        this->fileSize = file_size(filePath);
        this->progressCallBack = PrinterHasherCallBack(this->fileSize, this->printer);
      } catch (...) {
        return;
      }
    }

    virtual void update(uint64 justProcessed) {
      this->progressCallBack(justProcessed);
    }

    virtual void endFile(ArrayList<CheckError> errors) {
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

    virtual void end() {
      wCout << "  Errors: " << this->errorCount << endl;
      wCout << "  Ok: " << this->okCount << endl;
      wCout << endl;
    }

    PrinterCheckResultReporterImpl(arx::Printer* printer): printer(printer), progressCallBack(PrinterHasherCallBack((uint64) -1, this->printer)) {
      //this->progressCallBack = ;
    }
  };
}

PrinterCheckResultReporter::PrinterCheckResultReporter(arx::Printer* printer): CheckResultReporter(new ::detail::PrinterCheckResultReporterImpl(printer)) {
  return;
}


// -------------------------------------------------------------------------- //
// DelegatingHasherCallBack
// -------------------------------------------------------------------------- //
namespace detail {
  class DelegatingHasherCallBackImpl: public HasherCallBackImpl {
  private:
    CheckResultReporter reporter;

  public:
    DelegatingHasherCallBackImpl(CheckResultReporter reporter): reporter(reporter) {
      return;
    }

    virtual void operator() (uint64 justProcessed) {
      this->reporter.update(justProcessed);
    }

    virtual void clear() {
      return;
    }
  };
}

class DelegatingHasherCallBack: public HasherCallBack {
public:
  DelegatingHasherCallBack(CheckResultReporter reporter): HasherCallBack(new ::detail::DelegatingHasherCallBackImpl(reporter)) {
    return;
  }
};

// -------------------------------------------------------------------------- //
// LineCheckers
// -------------------------------------------------------------------------- //
class LineChecker {
public:
  virtual bool applicableTo(wstring line) = 0;
  virtual bool isDefaultFor(path fileName) = 0;
  virtual void parseLine(wstring line, Map<wstring, CheckTask> m) = 0;
};

class SimpleLineChecker: public LineChecker {
private:
  wregex lineRegex;
  wregex fileNameRegex;
  int hashMatchIndex;
  int fileNameMatchIndex;
  int sizeMatchIndex;
  bool sizeCheckNeeded;
  int hashId;

public:
  SimpleLineChecker(wstring lineRegex, wstring fileNameRegex, int hashMatchIndex, int fileNameMatchIndex, int hashId, bool sizeCheckNeeded, int sizeMatchIndex): 
    lineRegex(lineRegex), fileNameRegex(fileNameRegex), hashMatchIndex(hashMatchIndex), fileNameMatchIndex(fileNameMatchIndex), hashId(hashId), sizeCheckNeeded(sizeCheckNeeded), sizeMatchIndex(sizeMatchIndex) {
    return;
  }

  virtual bool applicableTo(wstring line) {
    return regex_match(line, this->lineRegex);
  }

  virtual bool isDefaultFor(path fileName) {
    return regex_match(to_lower_copy(fileName.filename().wstring()), this->fileNameRegex);
  }

  virtual void parseLine(wstring line, Map<wstring, CheckTask> m) {
    wsmatch match;
    regex_match(line, match, this->lineRegex);
    wstring fileName = trim_copy(wstring(match[this->fileNameMatchIndex].first, match[this->fileNameMatchIndex].second));
    string hash = toNarrowString(wstring(match[this->hashMatchIndex].first, match[this->hashMatchIndex].second));
#ifdef ARX_WIN
    wstring key = to_upper_copy(fileName);
#else
    wstring key = fileName;
#endif
    if(m.find(key) == m.end())
      m[key] = CheckTask(fileName);
    m[key].getRightEntry().setDigest(this->hashId, Digest::fromHexString(hash));
    m[key].getHashTask().add(this->hashId);
    if(sizeCheckNeeded) {
      m[key].setSizeNeedsChecking(true);
      m[key].getRightEntry().setSize(lexical_cast<uint64>(wstring(match[this->sizeMatchIndex].first, match[this->sizeMatchIndex].second)));
    }
  }
};

class BSDLineChecker: public LineChecker {
private:
  static wregex lineRegex;
public:
  virtual bool applicableTo(wstring line) {
    wsmatch match;
    if(regex_match(line, match, lineRegex))
      return Hash::getId(toNarrowString(wstring(match[1].first, match[1].second))) != H_UNKNOWN;
    return false;
  }

  virtual bool isDefaultFor(path fileName) {
    //return contains(to_lower_copy(fileName.native_file_string()), _T("bsd"));
    return true; // small cheat to prioritize bsd format over sfv one
  }

  virtual void parseLine(wstring line, Map<wstring, CheckTask> m) {
    wsmatch match;
    regex_match(line, match, lineRegex);
    wstring fileName = trim_copy(wstring(match[2].first, match[2].second));
    uint32 hashId = Hash::getId(toNarrowString(wstring(match[1].first, match[1].second)));
    string hash = toNarrowString(wstring(match[3].first, match[3].second));
#ifdef ARX_WIN
    wstring key = to_upper_copy(fileName);
#else
    wstring key = fileName;
#endif
    if(m.find(key) == m.end())
      m[key] = CheckTask(fileName);
    m[key].getRightEntry().setDigest(hashId, Digest::fromHexString(hash));
    m[key].getHashTask().add(hashId);
  }
};

wregex BSDLineChecker::lineRegex = wregex(_T("([a-zA-Z0-9]+)[ \t]*\\(") + fileNameRegex + _T("\\)[ \t]*=[ \t]*([0-9a-fA-F]+)"));

// -------------------------------------------------------------------------- //
// FileCheckers
// -------------------------------------------------------------------------- //
class FileChecker {
private:
  static ArrayList<FileChecker*> fileCheckers;
  static FileChecker* defaultChecker;

public:
  virtual bool applicableTo(path fileName) {
    return true;
  }

  virtual void check(path fileName, CheckResultReporter reporter) {
    for(int i = 0; i < fileCheckers.size(); i++) {
      if(fileCheckers[i]->applicableTo(fileName)) {
        fileCheckers[i]->check(fileName, reporter);
        return;
      }
    }
    defaultChecker->check(fileName, reporter);
  }

  static ArrayList<FileChecker*> constructFileCheckerList();
};

class TorrentFileChecker: public FileChecker {
public:
  virtual bool applicableTo(path fileName) {
    return ends_with(to_lower_copy(fileName.native()), ".torrent");
  }
  virtual void check(path fileName, CheckResultReporter reporter) {
    checkTorrent(fileName, reporter);
  }
};

class TextFileChecker: public FileChecker {
private:
  static ArrayList<LineChecker*> lineCheckers;

public:
  TextFileChecker() {
    if(lineCheckers.size() == 0) {
      lineCheckers.push_back(new SimpleLineChecker(fileNameRegex + _T("[ \t]+([0-9a-fA-F]{8})"),                                        _T("(.*crc.*)|(.*sfv.*)"), 2, 1, H_CRC,  false, -1));
      lineCheckers.push_back(new SimpleLineChecker(_T("ed2k://\\|file\\|") + fileNameRegex + _T("\\|([0-9]+)\\|([0-9a-fA-F]{32})\\|/"), _T(".*ed2k.*"),            3, 1, H_ED2K, true,   2));
      lineCheckers.push_back(new SimpleLineChecker(_T("([0-9a-fA-F]{32})[ \t\\*]+") + fileNameRegex,                                    _T(".*md5.*"),             1, 2, H_MD5,  false, -1));
      lineCheckers.push_back(new SimpleLineChecker(_T("([0-9a-fA-F]{40})[ \t\\*]+") + fileNameRegex,                                    _T(".*sha1.*"),            1, 2, H_SHA1, false, -1));
      lineCheckers.push_back(new BSDLineChecker());
    }
  }

  virtual bool applicableTo(path fileName) {
    return true;
  }

  virtual void check(path fileName, CheckResultReporter reporter) {
    path dir = fileName.branch_path();
    if(dir == _T(""))
      dir = _T(".");

    reporter.begin(fileName);

    Reader reader;
    try {
      reader = createReader(fileName, options.getInputEncoding(), false);
    } catch (...) {
      reporter.error(CheckError(CE_NOACCESS));
      reporter.end();
      return;
    }
    Scanner scanner(reader);

    Map<wstring, CheckTask> checkTasks;
    shared_array<bool> isApplicable = shared_array<bool>(new bool[lineCheckers.size()]);
    shared_array<bool> isDefaultFor = shared_array<bool>(new bool[lineCheckers.size()]);
    for(int i = 0; i < lineCheckers.size(); i++)
      isDefaultFor[i] = lineCheckers[i]->isDefaultFor(fileName);

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
        reporter.error(CheckError(CE_TOOMANYERRORS));
        break;
      }

      for(int i = 0; i < lineCheckers.size(); i++)
        isApplicable[i] = lineCheckers[i]->applicableTo(s);

      int applicableCount = 0;
      int applicableIndex = -1;
      for(int i = 0; i < lineCheckers.size(); i++) {
        if(isApplicable[i]) {
          applicableCount++;
          applicableIndex = i;
        }
      }

      if(applicableCount == 0) {
        reporter.error(CheckError(CE_WRONGFORMAT, lineN));
        criticalErrorCount++;
        continue;
      }

      if(applicableCount > 1) {
        applicableCount = 0;
        for(int i = 0; i < lineCheckers.size(); i++) {
          if(isApplicable[i] && isDefaultFor[i]) {
            applicableCount++;
            applicableIndex = i;
          }
        }
        if(applicableCount > 1 || applicableCount == 0) {
          reporter.error(CheckError(CE_WRONGFORMAT, lineN));
          criticalErrorCount++;
          continue;
        }
      }

      lineCheckers[applicableIndex]->parseLine(s, checkTasks);
    }

    for(Map<wstring, CheckTask>::iterator i = checkTasks.begin(); i != checkTasks.end(); i++) {
      path filePath = i->second.getRightEntry().getPath();
      wstring fileString = filePath.native();
      if(!filePath.is_complete())
        filePath = dir / filePath;

      reporter.beginFile(filePath, fileString);
      ArrayList<CheckError> errors;
      try {
        DelegatingHasherCallBack callBack(reporter);
        
        HashTask hashTask = i->second.getHashTask();
        FileEntry rightEntry = i->second.getRightEntry();
        FileEntry realEntry = FileEntry(rightEntry.getPath());

        Hasher hasher(hashTask, options.isMultiThreaded(), callBack);
        hasher.hash(realEntry);

        if(realEntry.isFailed())
          throw runtime_error("v_v");

        for(int hashId = 0; hashId < H_COUNT; hashId++) {
          if(hashTask.isSet(hashId)) {
            if(rightEntry.getDigest(hashId) != realEntry.getDigest(hashId))
              errors.push_back(CheckError(CE_WRONGHASH, hashId, rightEntry.getDigest(hashId), realEntry.getDigest(hashId)));
            if(i->second.isSizeNeedsChecking() && rightEntry.getSize() != file_size(filePath))
              errors.push_back(CheckError(CE_WRONGSIZE, rightEntry.getSize(), realEntry.getSize()));
          }
        }
      } catch (std::exception &) {
        errors.push_back(CheckError(CE_NOACCESS));
      }
      reporter.endFile(errors);
    }

    reporter.end();
  }
};
ArrayList<LineChecker*> TextFileChecker::lineCheckers;

ArrayList<FileChecker*> FileChecker::fileCheckers = FileChecker::constructFileCheckerList();
FileChecker* FileChecker::defaultChecker = new TextFileChecker();

ArrayList<FileChecker*> FileChecker::constructFileCheckerList() {
  ArrayList<FileChecker*> result;
  result.push_back(new TorrentFileChecker());
  return result;
}


// -------------------------------------------------------------------------- //
// CheckerImpl
// -------------------------------------------------------------------------- //
class Checker::CheckerImpl {
private:
  CheckResultReporter reporter;
  FileChecker checker;

public:
  CheckerImpl(CheckResultReporter reporter): reporter(reporter) {
    return;
  }

  void check(path filePath) {
    checker.check(filePath, reporter);
  }
};

// -------------------------------------------------------------------------- //
// Checker
// -------------------------------------------------------------------------- //
Checker::Checker(CheckResultReporter reporter): impl(new CheckerImpl(reporter)) {
  return;
}

void Checker::check(boost::filesystem::path filePath)  {
  this->impl->check(filePath);
}

