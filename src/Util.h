#ifndef __UTIL_H__
#define __UTIL_H__

#include <exception>
#include <string>

#if 0
class wruntime_error: std::exception {
private:
	string message;
public:
	explicit wruntime_error(const string& message): message(message) {}
	virtual ~wruntime_error()	{}
	virtual const char* what() const;
	virtual const wchar_t* wwhat() const;
};
#endif

#endif