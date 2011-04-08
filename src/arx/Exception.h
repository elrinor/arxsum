#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__
#include <exception>
#include <string>
#include <ostream>

namespace arx {

  class wruntime_error: public std::runtime_error {
  private:
    std::wstring errorMsg;

  public:
    wruntime_error(const std::wstring& errorMsg);
    wruntime_error(const wruntime_error& rhs);

    wruntime_error& operator=(const wruntime_error& rhs);

    virtual ~wruntime_error() throw();

    const std::wstring& getErrorMessage() const;
  } ;

  std::wostream& operator<< (std::wostream& stream, const std::exception& xcptn);
  std::ostream& operator<< (std::ostream& stream, const std::exception& xcptn);

} // namespace arx

#endif