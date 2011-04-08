#include "config.h"
#include "Converter.h"
#include <string>
#include <cassert>

#ifdef ARX_WIN
#  include <Windows.h>
#endif

#ifdef ARX_LINUX
#  include <iconv.h>
#endif

namespace arx {
  namespace detail {
// -------------------------------------------------------------------------- //
// Supplementary
// -------------------------------------------------------------------------- //
    bool isUTF16(const char* encoding) {
      const std::string s(encoding);
      if(s == "UTF-16" || s == "UTF16" || s == "UCS-2" || s == "UCS2")
        return true;
      return false;
    }

// -------------------------------------------------------------------------- //
// Interfaces
// -------------------------------------------------------------------------- //
    class ToUTF16ConverterInterface {
    public:
      virtual std::wstring convert(const std::string& src) const = 0;
      virtual std::wstring convertSome(std::string& src) const = 0;
    };

    class FromUTF16ConverterInterface {
    public:
      virtual std::string convert(const std::wstring& src) const = 0;
    };

// -------------------------------------------------------------------------- //
// Supplementary functions & classes
// -------------------------------------------------------------------------- //
#ifdef ARX_WIN
    std::string toUpper(const std::string& src) {
      std::string result = src;
      for(unsigned int i = 0; i < result.size(); i++)
        if(result[i] >= 'a' && result[i] <= 'z')
          result[i] = result[i] - 'a' + 'A';
      return result;
    }

    bool isNumber(const std::string& s) {
      for(unsigned int i = 0; i < s.size(); i++)
        if(s[i] < '0' || s[i] > '9')
          return false;
      return true;
    }

    bool startsWith(const std::string& s, const std::string& prefix) {
      if(prefix.size() > s.size())
        return false;
      for(unsigned int i = 0; i < prefix.size(); i++)
        if(s[i] != prefix[i])
          return false;
      return true;
    }

    std::string itoa(int n) {
      std::string result("");
      bool negative = false;
      if(n < 0) {
        negative = true;
        n = -n;
      }
      while(n != 0) {
        result = static_cast<char>('0' + n % 10) + result;
        n /= 10;
      }
      if(negative)
        result = "-" + result;
      return result;
    }

    unsigned int getCPId(const char* cpName) {
      const std::string c = toUpper(std::string(cpName));
      if(isNumber(cpName))
        return atoi(cpName);
      if((startsWith(c, "CP") || startsWith(c, "MS")) && isNumber(c.substr(2)))
        return atoi(c.substr(2).c_str());
      if(startsWith(c, "IBM") && isNumber(c.substr(3)))
        return atoi(c.substr(3).c_str());
      if(startsWith(c, "CSIBM") && isNumber(c.substr(5)))
        return atoi(c.substr(5).c_str());
      
      if(c == "DEFAULT")
        return CP_ACP;
      if(c == "CSPC8CODEPAGE437")
        return 437;
      if(c == "ASMO-708")
        return 708;
      if(c == "ASMO_449" || c == "CSISO89ASMO449")
        return 709;
      if(c == "CSPC775BALTIC")
        return 775;
      if(c == "CSPC850MULTILINGUAL")
        return 850;
      if(c == "CSPCP852")
        return 852;
      if(c == "IBM-932" || c == "CSSHIFTJIS" || c == "SHIFT-JIS" || c == "SHIFT_JIS" || c == "SJIS-OPEN" || c == "SJIS-WIN" || c == "SJIS")
        return 932;
      if(c == "WINDOWS-936") // ANSI/OEM - Simplified Chinese (PRC, Singapore)
        return 936;
      if(c == "MSCP949") // ANSI/OEM - Korean (Unified Hangul Code)
        return 949;
      if(c == "WINDOWS-1251" )
        return 1251;
      if(c == "CSKOI8R" || c == "KOI8-R" || c == "KOI8R")
        return 20866;
      if(c == "CSGB2312" || c == "GB2312") // Simplified Chinese (GB2312) 
        return 20936;
      if(c == "EUCJP" || c == "EUCJP-MS" || c == "EUCJP-OPEN" || c == "EUCJP-WIN" || c == "EUC-JP-MS" || c =="EUC-JP")
        return 51932;
      if(c == "UTF8" || c == "UTF-8")
        return 65001;
      throw std::runtime_error("Unknown encoding: " + c);
    }
#endif

#ifdef ARX_LINUX
    class IconvDeleter{
    public:
      void operator() (iconv_t* p) {
        if(*p != (iconv_t)(-1))
          iconv_close(*p);
        delete p;
      }
    };
#endif // ARX_LINUX

// -------------------------------------------------------------------------- //
// FromUTF16ConverterImpl
// -------------------------------------------------------------------------- //
    class FromUTF16ConverterImpl: public FromUTF16ConverterInterface {
#ifdef ARX_WIN
    private:
      unsigned int cpId;
    public:
      FromUTF16ConverterImpl(const char* encoding) {
        this->cpId = getCPId(encoding);
      }
      FromUTF16ConverterImpl() {
        this->cpId = CP_ACP;
      }
      std::string convert(const std::wstring& src) const {
        unsigned int n = WideCharToMultiByte(this->cpId, 0, src.c_str(), -1, NULL, 0, NULL, NULL);
        std::string result("", n);
        n = WideCharToMultiByte(this->cpId, 0, src.c_str(), -1, const_cast<char*>(result.c_str()), n, NULL, NULL);
        if(n == 0)
          throw std::runtime_error("Conversion from UTF16 failed");
        return std::string(result.c_str());
      }
#endif // ARX_WIN
#ifdef ARX_LINUX
    private:
      shared_ptr<iconv_t> conv;

      void init(const char* encoding) {
        string encs = encoding;
        if(encs == "DEFAULT")
          encs = "UTF-8";
        iconv_t* pIconv = new iconv_t;
        *pIconv = iconv_open(encs.c_str(), "WCHAR_T");
        if(*pIconv == (iconv_t)(-1)) {
          delete pIconv;
          throw std::runtime_error("Unknown encoding: " + encoding);
        }
        this->conv.reset(pIconv, IconvDeleter());
      }
    public:
      FromUTF16ConverterImpl(const char* encoding) {
        init(encoding);
      }
      FromUTF16ConverterImpl() {
        // TODO: is UTF-8 ok?
        init("UTF-8");
      }
      std::string convert(const std::wstring& src) const {
        char* inBuf = const_cast<char*>(src.c_str());
        size_t inBytesLeft = src.size();
        size_t outBytesLeft = inBytesLeft * 6 + 4; // Is that enough?
        std::string result("", outBytesLeft);
        char* outBuf = const_cast<char*>(result.c_str());
        size_t status = iconv(*this->conv, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft);
        if(status == (size_t)(-1) && errno != EINVAL)
          throw std::runtime_error("Conversion from UTF16 failed");
        return std::string(result.c_str());
      }
#endif // ARX_LINUX
    };


// -------------------------------------------------------------------------- //
// ToUTF16ConverterImpl
// -------------------------------------------------------------------------- //
    class ToUTF16ConverterImpl: public ToUTF16ConverterInterface {
#ifdef ARX_WIN
    private:
      unsigned int cpId;

    public:
      ToUTF16ConverterImpl(const char* encoding) {
        this->cpId = getCPId(encoding);
      }

      ToUTF16ConverterImpl() {
        this->cpId = CP_ACP;
      }

      std::wstring convert(const std::string& src) const {
        unsigned int n = MultiByteToWideChar(this->cpId, 0, src.c_str(), -1, NULL, 0);
        std::wstring result(L"", n);
        n = MultiByteToWideChar(this->cpId, 0, src.c_str(), -1, const_cast<wchar_t*>(result.c_str()), n);
        if(n == 0)
          throw std::runtime_error("Conversion to UTF16 failed");
        return std::wstring(result.c_str());
      }

      std::wstring convertSome(std::string& src) const {
        std::wstring result = convert(src);
        FromUTF16ConverterImpl conv(itoa(cpId).c_str());
        std::string back = conv.convert(result);
        unsigned int srcPos = 0;
        for(unsigned int i = 0; i < back.size(); i++) {
          while(true) {
            if(srcPos >= src.size()) {
              src.clear();
              return result;
            }
            if(src[srcPos] == back[i])
              break;
            srcPos++;
          }
          srcPos++;
        }
        src = src.substr(srcPos);
        return result;
      }

#endif // ARX_WIN

#ifdef ARX_LINUX
    private:
      shared_ptr<iconv_t> conv;

      void init(const char* encoding) {
        string encs = encoding;
        if(encs == "DEFAULT")
          encs = "UTF-8";
        iconv_t* pIconv = new iconv_t;
        *pIconv = iconv_open("WCHAR_T", encs.c_str());
        if(*pIconv == (iconv_t)(-1)) {
          delete pIconv;
          throw std::runtime_error("Unknown encoding: " + encoding);
        }
        this->conv.reset(pIconv, IconvDeleter());
      }

      std::wstring convert(std::string& src, bool replaceWithIncompleteSeq) {
        char* inBuf = const_cast<char*>(src.c_str());
        size_t inBytesLeft = src.size();
        size_t outBytesLeft = inBytesLeft * 4 + 4; // Is that enough?
        std::wstring result(L"", outBytesLeft / 2);
        char* outBuf = const_cast<char*>(result.c_str());
        while(true) {
          size_t status = iconv(*this->conv, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft);
          if(status == (size_t)(-1) && errno == EILSEQ) { // Invalid multibyte sequence
            inBuf++;
            inBytesLeft--;
          } else if(status != (size_t)(-1) || (status == (size_t)(-1) && errno == EINVAL)) {
            if(replaceWithIncompleteSeq)
              src = inBuf;
            return result;
          } else if(status == (size_t)(-1) && errno != EINVAL)
            throw std::runtime_error("Conversion to UTF16 failed");
        }
        return std::wstring(result.c_str());
      }

    public:
      ToUTF16ConverterImpl(const char* encoding) {
        init(encoding);
      }

      ToUTF16ConverterImpl() {
        // TODO: is UTF-8 ok?
        init("UTF-8");
      }

      std::wstring convert(const std::string& src) const {
        return convert(const_cast<std::string&>(src), false);
      }

      std::wstring convertSome(std::string& src) const {
        return convert(src, true);
      }

#endif // ARX_LINUX
    };

// -------------------------------------------------------------------------- //
// FallThrough Converters
// -------------------------------------------------------------------------- //
    class ToUTF16FallThroughConverterImpl: public ToUTF16ConverterInterface {
    public:
      std::wstring convert(const std::string& src) const {
        std::wstring result(L"", src.size() / sizeof(wchar_t));
        memcpy((void*) result.c_str(), src.c_str(), src.size() & (-(int) sizeof(wchar_t)));
        return result.c_str();
      }
      std::wstring convertSome(std::string& src) const {
        std::wstring result = convert(src);
        src = src.substr(result.size() * sizeof(wchar_t));
        return result;
      }
    };

    class FromUTF16FallThroughConverterImpl: public FromUTF16ConverterInterface {
    public:
      std::string convert(const std::wstring& src) const {
        std::string result("", src.size() * 2);
        memcpy((void*) result.c_str(), src.c_str(), src.size() * sizeof(wchar_t));
        return result;
      }
    };

  } // namespace detail

// -------------------------------------------------------------------------- //
// ToUTF16Converter
// -------------------------------------------------------------------------- //
  ToUTF16Converter::ToUTF16Converter(const char* encoding) {
    if(detail::isUTF16(encoding))
      this->impl.reset(new detail::ToUTF16FallThroughConverterImpl());
    else
      this->impl.reset(new detail::ToUTF16ConverterImpl(encoding));
  }

  ToUTF16Converter::ToUTF16Converter(): impl(new detail::ToUTF16ConverterImpl()) {}

  std::wstring ToUTF16Converter::convert(const std::string& src) const {
    return this->impl->convert(src);
  }

  std::wstring ToUTF16Converter::convertSome(std::string& src) const {
    return this->impl->convertSome(src);
  }

// -------------------------------------------------------------------------- //
// FromUTF16Converter
// -------------------------------------------------------------------------- //
  FromUTF16Converter::FromUTF16Converter(const char* encoding) {
    if(detail::isUTF16(encoding))
      this->impl.reset(new detail::FromUTF16FallThroughConverterImpl());
    else
      this->impl.reset(new detail::FromUTF16ConverterImpl(encoding));
  }

  FromUTF16Converter::FromUTF16Converter(): impl(new detail::FromUTF16ConverterImpl()) {}

  std::string FromUTF16Converter::convert(const std::wstring& src) const {
    return this->impl->convert(src);
  }

// -------------------------------------------------------------------------- //
// Conversion functions
// -------------------------------------------------------------------------- //
  std::wstring toWideString(const std::string& str) {
    ToUTF16Converter conv;
    return conv.convert(str);
  }

  std::wstring toWideString(const std::string& str, const char* encoding) {
    ToUTF16Converter conv(encoding);
    return conv.convert(str);
  }

  std::string toNarrowString(const std::wstring& str) {
    FromUTF16Converter conv;
    return conv.convert(str);
  }

  std::string toNarrowString(const std::wstring& str, const char* encoding) {
    FromUTF16Converter conv(encoding);
    return conv.convert(str);
  }

} // namespace arx

