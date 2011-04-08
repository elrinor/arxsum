#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#include "config.h"
#include <boost/shared_ptr.hpp>
#include <string>
#include "arx/Streams.h"
#include "FileEntry.h"
#include "ArrayList.h"
#include "Task.h"

enum {
  O_SFV,
  O_MD5,
  O_ED2K,
  O_SHA1,
  O_BSD,
  O_COUNT,
  O_UNKNOWN = 0xFFFFFFFF
};

extern const uint32 outTaskRequirement[];

namespace detail {
  class OutputFormatImpl;
};

class OutputFormat {
private:
  boost::shared_ptr<::detail::OutputFormatImpl> impl;
  uint32 ofId;
public:
  OutputFormat(uint32 ofId);
  void output(HashTask task, ArrayList<FileEntry> data, arx::Printer* printer);
  std::string getName();
  uint32 getId();
  static std::string getName(uint32 ofId);
};

#endif
