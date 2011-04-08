#include "config.h"
#include "Util.h"
#include <locale>

#if 0
const char* wruntime_error::what() const {
	std::codecvt<wchar_t, char, mbstate_t> cvt;
	cvt.do_out()
	return (_Str.c_str());
}

const wchar_t* wruntime_error::wwhat() const {
	return message.c_str();
}

#endif
