#include "Exception.h"
#include "Converter.h"

namespace arx {

  wruntime_error::wruntime_error(const std::wstring& errorMsg): runtime_error(toNarrowString(errorMsg)), errorMsg(errorMsg) {
    return;
  }

  wruntime_error::wruntime_error(const wruntime_error& rhs): runtime_error(toNarrowString(rhs.getErrorMessage())), errorMsg(rhs.getErrorMessage()) {
    return;
  }

  wruntime_error& wruntime_error::operator=(const wruntime_error& rhs) {
    runtime_error::operator=(rhs); 
    errorMsg = rhs.errorMsg; 
    return *this; 
  }

  wruntime_error::~wruntime_error() throw() {
    return;
  }

  const std::wstring& wruntime_error::getErrorMessage() const {
    return this->errorMsg;
  }

  std::wostream& operator<< (std::wostream& stream, const std::exception& xcptn) {
    if(const wruntime_error* p = dynamic_cast<const wruntime_error*>(&xcptn))
      stream << p->getErrorMessage();
    else 
      stream << toWideString(xcptn.what());
    return stream;
  }

  std::ostream& operator<< (std::ostream& stream, const std::exception& xcptn) {
    stream << xcptn.what();
    return stream;
  }


} // namespace arx