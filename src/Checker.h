#ifndef __CHECKER_H__
#define __CHECKER_H__

#include "config.h"
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include "arx/Collections.h"
#include "arx/Streams.h"
#include "Hash.h"
#include "Hasher.h"

// -------------------------------------------------------------------------- //
// CheckError
// -------------------------------------------------------------------------- //
enum CheckErrorType {
  CE_WRONGHASH,
  CE_WRONGSIZE,
  CE_NOACCESS,
  CE_WRONGFORMAT,
  CE_TOOMANYERRORS,
  CE_NOERROR = -1
};

class CheckError {
public:
  CheckErrorType getType();

  int getHashId();
  Digest getRightHash();
  Digest getWrongHash();

  uint64 getRightSize();
  uint64 getWrongSize();

  uint32 getLineNumber();

  CheckError(CheckErrorType type);
  CheckError(CheckErrorType type, uint32 lineNumber);
  CheckError(CheckErrorType type, uint64 rightSize, uint64 wrongSize);
  CheckError(CheckErrorType type, int hashId, Digest rightHash, Digest wrongHash);

private:
  class CheckErrorImpl;
  boost::shared_ptr<CheckErrorImpl> impl;
};


// -------------------------------------------------------------------------- //
// CheckResultReporter
// -------------------------------------------------------------------------- //
class CheckResultReporter {
public:
  virtual void error(CheckError error);
  virtual void error(std::wstring errorString);
  virtual void begin(const boost::filesystem::wpath& checkSumFilePath);
  virtual void beginFile(const boost::filesystem::wpath& filePath, const std::wstring& fileString);
  virtual void update(uint64 justProcessed);
  virtual void endFile(arx::ArrayList<CheckError> errors);
  virtual void end();
};


// -------------------------------------------------------------------------- //
// PrinterCheckResultReporter
// -------------------------------------------------------------------------- //
class PrinterCheckResultReporter: public CheckResultReporter {
private:
  arx::Printer* printer;
  std::wstring checkSumFileName;
  std::wstring fileName;
  uint64 fileSize;
  uint32 okCount, errorCount;
  StreamOutHasherCallBack progressCallBack;

public:
  virtual void error(CheckError error);
  virtual void error(std::wstring errorString);
  virtual void begin(const boost::filesystem::wpath& checkSumFilePath);
  virtual void beginFile(const boost::filesystem::wpath& filePath, const std::wstring& fileString);
  virtual void update(uint64 justProcessed);
  virtual void endFile(arx::ArrayList<CheckError> errors);
  virtual void end();

  PrinterCheckResultReporter(arx::Printer* printer);
};


// -------------------------------------------------------------------------- //
// Checker
// -------------------------------------------------------------------------- //
class Checker {
private:
  class CheckerImpl;
  boost::shared_ptr<CheckerImpl> impl;
public:
  Checker(boost::shared_ptr<CheckResultReporter> reporter = boost::shared_ptr<CheckResultReporter>());
  void check(boost::filesystem::wpath filePath);
};


#endif