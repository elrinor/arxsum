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

namespace detail {
  class HasherImpl;
}

class HasherCallBack {
public:
  virtual void operator() (uint64 justProcessed) = 0;
};

class StreamOutHasherCallBack: public HasherCallBack {
private:
  uint64 sumSize;
  uint64 processed;
  uint64 lastProcessed;
  boost::timer t;
  double lastOutputTime;
  arx::Printer* printer;
public:
  StreamOutHasherCallBack() {}
  StreamOutHasherCallBack(uint64 sumSize, arx::Printer* printer): sumSize(sumSize), processed(0), lastProcessed(0), lastOutputTime(0.0), printer(printer) {}
  void operator() (uint64 justProcessed);
  void clear();
};

class Hasher: private boost::noncopyable {
private:
  boost::shared_ptr<::detail::HasherImpl> impl;
public:
  Hasher(HashTask task, bool isMultiThreaded, boost::shared_ptr<HasherCallBack> callBack = boost::shared_ptr<HasherCallBack>());
  void hash(FileEntry entry);
  static Digest hash(uint32 hashId, boost::filesystem::wpath filePath, boost::shared_ptr<HasherCallBack> callBack);
};

#endif
