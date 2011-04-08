#ifndef __HASHER_H__
#define __HASHER_H__

#include "config.h"
#include "Task.h"
#include "FileEntry.h"
#include "arx/Streams.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/timer.hpp>
#include <boost/filesystem.hpp>

// -------------------------------------------------------------------------- //
// HasherCallBack
// -------------------------------------------------------------------------- //
namespace detail {
  class HasherCallBackImpl {
  public:
    virtual void operator() (uint64 justProcessed) {return;};
    virtual void clear() {return;};
  };
}

class HasherCallBack {
private:
  boost::shared_ptr<::detail::HasherCallBackImpl> impl;

protected:
  HasherCallBack(::detail::HasherCallBackImpl* impl);

public:
  HasherCallBack();

  void operator() (uint64 justProcessed);
  void clear();
};

// -------------------------------------------------------------------------- //
// PrinterHasherCallBack
// -------------------------------------------------------------------------- //
class PrinterHasherCallBack: public HasherCallBack {
public:
  PrinterHasherCallBack(uint64 sumSize, arx::Printer* printer);
};

// -------------------------------------------------------------------------- //
// Hasher
// -------------------------------------------------------------------------- //
namespace detail {
  class HasherImpl;
}

class Hasher {
private:
  boost::shared_ptr<::detail::HasherImpl> impl;
public:
  Hasher(HashTask task, bool isMultiThreaded, HasherCallBack callBack = HasherCallBack());
  void hash(FileEntry entry);
  static Digest hash(uint32 hashId, boost::filesystem::wpath filePath, HasherCallBack callBack);
};

#endif
