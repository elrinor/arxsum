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
private:
  class CheckErrorImpl;
  boost::shared_ptr<CheckErrorImpl> impl;

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
};


// -------------------------------------------------------------------------- //
// CheckResultReporter
// -------------------------------------------------------------------------- //
namespace detail {
  class CheckResultReporterImpl;
}

class CheckResultReporter {
private:
  boost::shared_ptr<::detail::CheckResultReporterImpl> impl;

protected:
  CheckResultReporter(::detail::CheckResultReporterImpl* impl);

public:
  CheckResultReporter();

  void error(CheckError error);
  void error(std::wstring errorString);
  void begin(const boost::filesystem::wpath& checkSumFilePath);
  void beginFile(const boost::filesystem::wpath& filePath, const std::wstring& fileString);
  void update(uint64 justProcessed);
  void endFile(arx::ArrayList<CheckError> errors);
  void end();
};

// -------------------------------------------------------------------------- //
// PrinterCheckResultReporter
// -------------------------------------------------------------------------- //
class PrinterCheckResultReporter: public CheckResultReporter {
public:
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
  Checker(CheckResultReporter reporter = CheckResultReporter());
  void check(boost::filesystem::wpath filePath);
};


#endif